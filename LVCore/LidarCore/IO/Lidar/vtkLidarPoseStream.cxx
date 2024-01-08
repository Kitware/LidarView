#include "vtkLidarPoseStream.h"
#include "vtkHelper.h"

#include <sstream>

#include <vtkCellArray.h>
#include <vtkInformationVector.h>
#include <vtkInformation.h>
#include <vtkLine.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyLine.h>
#include <vtkVariantArray.h>

namespace {
void DeepReverseCopy(vtkTable* input, vtkTable* output)
{
  vtkIdType nbCol = input->GetNumberOfColumns();
  vtkIdType nbRow = input->GetNumberOfRows();
  // copy array
  for (vtkIdType i = 0; i < nbCol; ++i)
  {
    auto* original = input->GetColumn(i);

    // handle only case with one element
    if (original->GetNumberOfComponents() != 1)
    {
      vtkGenericWarningMacro(<< "Currently only array with one componenets are supported, "
                             << "this is not the case of array :" << original->GetName());
      continue;
    }

    auto* array = original->NewInstance();
    array->SetName(original->GetName());
    array->SetNumberOfValues(original->GetNumberOfValues());
    for (vtkIdType j = 0; j < nbRow; ++j)
    {
      auto inv = (nbRow -1) - j;
      array->SetVariantValue(j,original->GetVariantValue(inv));
    }
    output->AddColumn(array);
  }
}
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarPoseStream)

//-----------------------------------------------------------------------------
vtkLidarPoseStream::vtkLidarPoseStream()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
  this->AllPositionsOrientation = vtkSmartPointer<vtkPolyData>::New();
  this->AllRawInformation = vtkSmartPointer<vtkTable>::New();
  this->LastNumberPositionOrientationInformation = 0;
  this->LastNumberRawInformation = 0;
}

//-----------------------------------------------------------------------------
vtkLidarPoseStream::~vtkLidarPoseStream()
{
  // see the explanation about why this is needed in vtkStream::~vtkStream
  this->Stop();
}

//-----------------------------------------------------------------------------
int vtkLidarPoseStream::FillOutputPortInformation(int port, vtkInformation* info)
{
  if ( port == 0 )
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    return 1;
  }

  if ( port == 1 )
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable" );
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkLidarPoseStream::RequestData(vtkInformation* vtkNotUsed(request),
                                vtkInformationVector** vtkNotUsed(inputVector),
                                vtkInformationVector* outputVector)
{
  {
    std::lock_guard<std::mutex> lock(this->DataMutex);

    vtkPolyData* outputPositionsOrientations = vtkPolyData::GetData(outputVector, 0);
    outputPositionsOrientations->DeepCopy(this->AllPositionsOrientation);
    this->LastNumberPositionOrientationInformation = this->AllPositionsOrientation->GetNumberOfPoints();


    vtkTable* outputRawInformation = vtkTable::GetData(outputVector, 1);
    DeepReverseCopy(this->AllRawInformation, outputRawInformation);
    this->LastNumberRawInformation = this->AllRawInformation->GetNumberOfRows();

  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::AddNewData()
{
  if (this->GetPoseInterpreter()->HasRawInformation())
  {
    if(this->AllRawInformation->GetNumberOfRows() == 0)
    {
      this->AllRawInformation->DeepCopy(this->GetPoseInterpreter()->GetRawInformation());
      return;
    }

    vtkSmartPointer<vtkTable> raw = this->GetPoseInterpreter()->GetRawInformation();

    for(int i = 0; i < raw->GetNumberOfRows(); i++)
    {
      vtkVariantArray * array = raw->GetRow(i);
      this->AllRawInformation->InsertNextRow(array);
    }
  }

  if (this->GetPoseInterpreter()->HasPositionOrientationInformation())
  {
    if(this->AllPositionsOrientation->GetNumberOfPoints() == 0)
    {
      this->AllPositionsOrientation->DeepCopy(
        this->GetPoseInterpreter()->GetPositionOrientation());
      return;
    }
    // Copying the new Position Orientation information (points, rows, ...) presents in the
    // interpreter to the corresponding buffer
    vtkSmartPointer<vtkPolyData> posOr = this->GetPoseInterpreter()->GetPositionOrientation();
    vtkPoints* points = this->AllPositionsOrientation->GetPoints();
    for(vtkIdType i = 0; i < posOr->GetNumberOfPoints(); i++)
    {
      for(vtkIdType j = 0; j < posOr->GetPointData()->GetNumberOfArrays(); j++)
      {
        // Insert the i-th tuple of the current array (source) to the corresponding array of the bufferize vtkPolyData
        vtkAbstractArray * source = posOr->GetPointData()->GetAbstractArray(j);
        this->AllPositionsOrientation->GetPointData()->GetAbstractArray(j)->InsertNextTuple(i, source);
      }
      // Update points
      double pointToAdd[3];
      posOr->GetPoint(i, pointToAdd);
      points->InsertNextPoint(pointToAdd);
      this->AllPositionsOrientation->SetPoints(points);
    }

    // Set the polyline to the poly data to see the position orientation information
    vtkSmartPointer<vtkPolyLine> polyLine = CreatePolyLineFromPoints(this->AllPositionsOrientation->GetPoints());
    vtkNew<vtkCellArray> cellArray;
    cellArray->InsertNextCell(polyLine);
    this->AllPositionsOrientation->SetLines(cellArray);
  }
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::ClearAllDataAvailable()
{
  this->GetPoseInterpreter()->ResetCurrentData();
}

//----------------------------------------------------------------------------
int vtkLidarPoseStream::CheckForNewData()
{
  return this->CheckNewDataPositionOrientation() + this->CheckForNewDataRawInformation();
}

//----------------------------------------------------------------------------
int vtkLidarPoseStream::CheckNewDataPositionOrientation()
{
  return this->AllPositionsOrientation->GetNumberOfPoints() - this->LastNumberPositionOrientationInformation;
}

//----------------------------------------------------------------------------
int vtkLidarPoseStream::CheckForNewDataRawInformation()
{
  return this->AllRawInformation->GetNumberOfRows() - this->LastNumberRawInformation;
}

//----------------------------------------------------------------------------
vtkLidarPosePacketInterpreter* vtkLidarPoseStream::GetPoseInterpreter()
{
  return vtkLidarPosePacketInterpreter::SafeDownCast(this->Interpreter);
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetPoseInterpreter(vtkLidarPosePacketInterpreter* interpreter)
{
  this->Interpreter = interpreter;
}

