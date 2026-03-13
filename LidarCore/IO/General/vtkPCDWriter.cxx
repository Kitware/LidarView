#include "vtkPCDWriter.h"

#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTemplateAliasMacro.h>

#include <unordered_set>
#include <sstream>

using namespace std;

namespace {
// helper function as std::endian is a C++20 feature
bool is_big_endian(void)
{
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1;
}

}
//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPCDWriter)

//-----------------------------------------------------------------------------
vtkPCDWriter::~vtkPCDWriter()
{
  delete[] this->FileName;
}

//-----------------------------------------------------------------------------
void vtkPCDWriter::WriteData()
{
  auto* polyData = vtkPolyData::SafeDownCast(this->Superclass::GetInput());

  if (!polyData || (polyData && polyData->GetNumberOfPoints() == 0))
  {
    vtkErrorMacro("No points to save");
    return;
  }

  // check endianness as binary PCD file should be in little-endian
  if (this->Binary && is_big_endian())
  {
    vtkErrorMacro("Binary mode is not supported on big_endian architecture");
    return;
  }

  if(!this->FileName)
  {
    vtkErrorMacro("FileName is empty");
    return;
  }

  ofstream file(this->FileName);
  if (!file.is_open())
  {
    vtkErrorMacro("Could not open file: " << this->FileName);
    return;
  }

  this->WriteHeader(polyData, file);
  this->WriteBody(polyData, file);
  file.close();
}


//-----------------------------------------------------------------------------
void vtkPCDWriter::WriteHeader(vtkPolyData* polydata, ofstream& file)
{
  vtkPointData* pd = polydata->GetPointData();

  file << "# .PCD v0.7 - Point Cloud Data file format";
  file << "\nVERSION 0.7";

  // gather information about the different arrays
  stringstream fields, sizes, types, counts;

  // add x, y, z from the vtkPoints
  // a lot of other tools rely on the existence of float fields named x y z
  // so use this conviention even if it isn"t specify by the standart
  if (polydata->GetPoints()->GetDataType() == VTK_DOUBLE)
  {
    vtkWarningMacro("Possible loss of precision on (x, y, z) as they will be stored on 32 bits instead of 64 bits");
  }
  fields << "x y z";
  sizes  << "4 4 4";
  types  << "F F F";
  counts << "1 1 1";

  for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
  {
    auto* array = pd->GetArray(i);

    // determine TYPE
    char type;
    switch (array->GetDataType()) {
      case VTK_SIGNED_CHAR:
      case VTK_SHORT:
      case VTK_INT:
      case VTK_LONG:
      case VTK_LONG_LONG: type = 'I';
                          break;
      case VTK_UNSIGNED_CHAR:
      case VTK_UNSIGNED_SHORT:
      case VTK_UNSIGNED_INT:
      case VTK_UNSIGNED_LONG:
      case VTK_UNSIGNED_LONG_LONG: type = 'U';
                                   break;
      case VTK_TYPE_FLOAT32:
      case VTK_TYPE_FLOAT64: type = 'F';
                             break;
    default:
      vtkWarningMacro("Unsupported type for array " << array->GetName());
      continue;
    }

    // determine FIELDS
    unordered_set<string> reservedNamed = { "x", "y", "z"};
    string arrayName = pd->GetArrayName(i);
    if (reservedNamed.count(arrayName))
    {
      vtkWarningMacro("Postfixed array named " << arrayName << " with '_array' as arrays "
                      "named 'x' 'y' and 'z' are reserved for point coordinates");
      arrayName.append("_array");
    }

    fields << " " << arrayName;
    sizes << " " << array->GetDataTypeSize();
    types << " " << type;
    counts << " " << array->GetNumberOfComponents();
  }
  file << "\nFIELDS " << fields.str();
  file << "\nSIZE " << sizes.str();
  file << "\nTYPE " << types.str();
  file << "\nCOUNT " << counts.str();

  file << "\nWIDTH " << pd->GetNumberOfTuples();
  file << "\nHEIGHT " << 1;
  file << "\nVIEWPOINT " << "0 0 0 1 0 0 0";
  file << "\nPOINTS " << pd->GetNumberOfTuples();

  string mode = this->Binary ? "binary" : "ascii";
  file << "\nDATA " << mode << "\n";
}

//-----------------------------------------------------------------------------
void vtkPCDWriter::WriteBody(vtkPolyData* polydata, ofstream& file)
{
  // If speed up is needed, and you can make the assumption the array are AOS
  // do not hesite to grap directly the data in memory using using GetVoidPointer()
  vtkPointData* pd = polydata->GetPointData();
  if (this->Binary)
  {
    for (int i = 0; i < polydata->GetNumberOfPoints(); i++)
    {
      // Write x y z
      double pt[3];
      polydata->GetPoint(i, pt);
      float x = static_cast<float>(pt[0]);
      float y = static_cast<float>(pt[1]);
      float z = static_cast<float>(pt[2]);
      file.write(reinterpret_cast<const char *>(&x), sizeof(float));
      file.write(reinterpret_cast<const char *>(&y), sizeof(float));
      file.write(reinterpret_cast<const char *>(&z), sizeof(float));

      // Write other array
      for (int j = 0; j < pd->GetNumberOfArrays(); ++j)
      {
        auto* array = pd->GetArray(j);
        int dataType = array->GetDataType();
        for (int k = 0; k < array->GetNumberOfComponents(); ++k)
        {
          switch (dataType)
          {
            vtkTemplateAliasMacro(
              VTK_TT data = static_cast<VTK_TT>(array->GetComponent(i,k));
              file.write(reinterpret_cast<const char *>(&data), sizeof(VTK_TT));
            );
            default:
              vtkErrorMacro("unknow vtk type encountered in array " << array->GetName());
          }
        }
      }
    }
  }
  else
  {
    for (int i = 0; i < polydata->GetNumberOfPoints(); i++)
    {
      // configure the stream to handle float as desired
      file << std::fixed;
      file.precision(this->FloatPointPrecision);

      // Write x y z
      double pt[3] ;
      polydata->GetPoint(i, pt);
      file << static_cast<float>(pt[0]) << " "
           << static_cast<float>(pt[1]) << " "
           << static_cast<float>(pt[2]);

      // Write other array
      for (int j = 0; j < pd->GetNumberOfArrays(); ++j)
      {
        auto* array = pd->GetArray(j);
        int dataType = array->GetDataType();
        for (int k = 0; k < array->GetNumberOfComponents(); ++k)
        {
          if(dataType == VTK_UNSIGNED_CHAR)
          {
            file << " " << static_cast<int>(array->GetComponent(i,k));
          }
          else
          {
            switch (dataType)
            {
              vtkTemplateAliasMacro(file << " " << static_cast<VTK_TT>(array->GetComponent(i,k)));
              default:
                vtkErrorMacro("unknow vtk type encountered in array " << array->GetName());
            }
          }
        }
      }
      file << "\n";
    }
  }
}

//-----------------------------------------------------------------------------
int vtkPCDWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
