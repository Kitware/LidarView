/*=========================================================================

  Program: LidarView
  Module:  vtkLASWriter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLASWriter.h"

#include <filesystem>

#include <vtkAbstractArray.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkType.h>
#include <vtkVariant.h>
#include <vtksys/SystemTools.hxx>

#include <pdal/Dimension.hpp>
#include <pdal/Options.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/io/BufferReader.hpp>

vtkStandardNewMacro(vtkLASWriter);

namespace
{
//------------------------------------------------------------------------------
std::string normalizePath(const std::string& oldPath)
{
  std::filesystem::path path(oldPath);
  std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
  return canonicalPath.make_preferred().string();
}

//------------------------------------------------------------------------------
pdal::Dimension::Id vtkArrayToPDAL(const std::string arrayName)
{
  static const std::map<std::string, pdal::Dimension::Id> pdalMapping{
    { "intensity", pdal::Dimension::Id::Intensity },
    { "adjustedtime", pdal::Dimension::Id::GpsTime }
  };

  auto itr = pdalMapping.find(arrayName);
  if (itr != pdalMapping.end())
  {
    return itr->second;
  }
  return pdal::Dimension::Id::Unknown;
}

//------------------------------------------------------------------------------
pdal::Dimension::Type vtkTypeToPDAL(int type)
{
  switch (type)
  {
    case VTK_CHAR:
      return pdal::Dimension::Type::Signed8;
    case VTK_SHORT:
      return pdal::Dimension::Type::Signed16;
    case VTK_TYPE_INT32:
      return pdal::Dimension::Type::Signed32;
    case VTK_TYPE_INT64:
      return pdal::Dimension::Type::Signed64;
    case VTK_UNSIGNED_CHAR:
      return pdal::Dimension::Type::Unsigned8;
    case VTK_UNSIGNED_SHORT:
      return pdal::Dimension::Type::Unsigned16;
    case VTK_TYPE_UINT32:
      return pdal::Dimension::Type::Unsigned32;
    case VTK_TYPE_UINT64:
      return pdal::Dimension::Type::Unsigned64;
    case VTK_FLOAT:
      return pdal::Dimension::Type::Float;
    case VTK_DOUBLE:
      return pdal::Dimension::Type::Double;
    default:
      return pdal::Dimension::Type::None;
  }
}

//------------------------------------------------------------------------------
bool isIgnoredArray(const std::string arrayName)
{
  static const std::vector<std::string> toIgnore{ "x", "y", "z" };

  std::string stringLower;
  std::transform(arrayName.cbegin(),
    arrayName.cend(),
    std::back_inserter(stringLower),
    [](unsigned char c) { return std::tolower(c); });

  return std::find(toIgnore.cbegin(), toIgnore.cend(), stringLower) != toIgnore.cend();
}
}

//------------------------------------------------------------------------------
vtkLASWriter::vtkLASWriter()
{
  this->FileName = nullptr;
}

//------------------------------------------------------------------------------
vtkLASWriter::~vtkLASWriter()
{
  delete[] this->FileName;
}

//------------------------------------------------------------------------------
void vtkLASWriter::WriteData()
{
  vtkPolyData* input = this->GetInput();
  vtkPoints* pts = input->GetPoints();
  vtkPointData* ptsData = input->GetPointData();
  vtkIdType npts = pts->GetNumberOfPoints();

  if (npts <= 0)
  {
    vtkErrorMacro("No points are available in source polydata.");
    return;
  }

  int arrayNb = ptsData->GetNumberOfArrays();
  std::vector<pdal::Dimension::Id> pdalMapId(arrayNb, pdal::Dimension::Id::Unknown);

  int dataformat_id = 0;
  pdal::PointTable table;
  table.layout()->registerDim(pdal::Dimension::Id::X);
  table.layout()->registerDim(pdal::Dimension::Id::Y);
  table.layout()->registerDim(pdal::Dimension::Id::Z);
  for (int i = 0; i < arrayNb; ++i)
  {
    std::string arrayName = ptsData->GetAbstractArray(i)->GetName();
    if (arrayName == "Color" && ptsData->GetAbstractArray(i)->GetNumberOfComponents() == 3)
    {
      table.layout()->registerDim(pdal::Dimension::Id::Red);
      table.layout()->registerDim(pdal::Dimension::Id::Green);
      table.layout()->registerDim(pdal::Dimension::Id::Blue);
      dataformat_id += 2;
    }

    pdal::Dimension::Type pdalType = ::vtkTypeToPDAL(ptsData->GetAbstractArray(i)->GetDataType());
    if (ptsData->GetAbstractArray(i)->GetNumberOfComponents() != 1 ||
      pdalType == pdal::Dimension::Type::None || isIgnoredArray(arrayName))
    {
      continue;
    }

    if ((pdalMapId[i] = ::vtkArrayToPDAL(arrayName)) != pdal::Dimension::Id::Unknown)
    {
      table.layout()->registerDim(pdalMapId[i]);
      if (pdalMapId[i] == pdal::Dimension::Id::GpsTime)
      {
        dataformat_id += 1;
      }
    }
    else
    {
      pdalMapId[i] = table.layout()->registerOrAssignDim(arrayName, pdalType);
    }
  }

  pdal::Options options;

  std::string filename(this->FileName);
  if (this->Compression && vtksys::SystemTools::GetFilenameLastExtension(filename) == ".las")
  {
    // Replace .las with .laz
    filename.replace(filename.size() - 1, 1, "z");
  }
  options.add("filename", ::normalizePath(filename));

  // 0 == no color or time stored
  // 1 == time is stored
  // 2 == color is stored
  // 3 == color and time are stored
  options.add("dataformat_id", dataformat_id);

  // By default it truncate positions from .01
  options.add("scale_x", "auto");
  options.add("scale_y", "auto");
  options.add("scale_z", "auto");
  options.add("offset_x", this->Offset[0]);
  options.add("offset_y", this->Offset[1]);
  options.add("offset_z", this->Offset[2]);

  // If true generate a laz file instead
  options.add("compression", this->Compression);

  // Options to support extra dims
  options.add("extra_dims", "all");
  options.add("minor_version", "4");

  if (!this->SRSInformation.empty())
  {
    options.add("a_srs", this->SRSInformation);
  }

  pdal::PointViewPtr view = std::make_shared<pdal::PointView>(table);

  vtkDataArray* arr = pts->GetData();
  for (vtkIdType i = 0; i < npts; ++i)
  {
    view->setField(pdal::Dimension::Id::X, i, arr->GetComponent(i, 0));
    view->setField(pdal::Dimension::Id::Y, i, arr->GetComponent(i, 1));
    view->setField(pdal::Dimension::Id::Z, i, arr->GetComponent(i, 2));
    for (int j = 0; j < arrayNb; ++j)
    {
      vtkDataArray* array = vtkDataArray::SafeDownCast(ptsData->GetAbstractArray(j));
      if (strcmp(array->GetName(), "Color") == 0 && array->GetNumberOfComponents() == 3)
      {
        view->setField(pdal::Dimension::Id::Red, i, array->GetComponent(i, 0));
        view->setField(pdal::Dimension::Id::Green, i, array->GetComponent(i, 1));
        view->setField(pdal::Dimension::Id::Blue, i, array->GetComponent(i, 2));
      }
      else if (pdalMapId[j] != pdal::Dimension::Id::Unknown)
      {
        view->setField(pdalMapId[j], i, array->GetComponent(i, 0));
      }
    }
  }

  pdal::BufferReader reader;
  reader.addView(view);

  pdal::StageFactory factory;
  pdal::Stage* writer = factory.createStage("writers.las");

  try
  {
    writer->setInput(reader);
    writer->setOptions(options);
    writer->prepare(table);
    writer->execute(table);
  }
  catch (const pdal::pdal_error& err)
  {
    vtkErrorMacro("Failed to write LAS file with following pdal error \"" << err.what() << "\"");
  }
}

//------------------------------------------------------------------------------
void vtkLASWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName " << this->FileName << endl;
  os << indent << "SRSInformation " << this->SRSInformation << endl;
  os << indent << "Offset " << this->Offset[0] << " " << this->Offset[1] << " " << this->Offset[2]
     << endl;
  os << indent << "Compression " << this->Compression << endl;
}

//------------------------------------------------------------------------------
vtkPolyData* vtkLASWriter::GetInput()
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
vtkPolyData* vtkLASWriter::GetInput(int port)
{
  return vtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

//------------------------------------------------------------------------------
int vtkLASWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
