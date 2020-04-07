#include "vtkPositionOrientationStream.h"

#include <sstream>

#include "NetworkSource.h"
#include "PacketConsumer.h"
#include "PacketFileWriter.h"

#include <vtkInformationVector.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkVariantArray.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPositionOrientationStream)

//-----------------------------------------------------------------------------
vtkPositionOrientationStream::vtkPositionOrientationStream()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);
  this->AllPositionsOrientation = vtkSmartPointer<vtkPolyData>::New();
  this->AllRawInformation = vtkSmartPointer<vtkTable>::New();
}

//-----------------------------------------------------------------------------
int vtkPositionOrientationStream::FillOutputPortInformation(int port, vtkInformation* info)
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
int vtkPositionOrientationStream::RequestData(vtkInformation* vtkNotUsed(request),
                                vtkInformationVector** vtkNotUsed(inputVector),
                                vtkInformationVector* outputVector)
{
  {
    boost::lock_guard<boost::mutex> lock(this->Consumer->ConsumerMutex);

    vtkPolyData* outputPositionsOrientations = vtkPolyData::GetData(outputVector, 0);
    outputPositionsOrientations->DeepCopy(this->AllPositionsOrientation);
    this->LastNumberPositionOrientationInformation = this->AllPositionsOrientation->GetNumberOfPoints();


    vtkTable* outputRawInformation = vtkTable::GetData(outputVector, 1);
    outputRawInformation->DeepCopy(this->AllRawInformation);
    this->LastNumberRawInformation = this->AllRawInformation->GetNumberOfRows();

  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPositionOrientationStream::AddNewData()
{
  if(this->PositionOrientationInterpreter->HasRawInformation())
  {
    if(this->AllRawInformation->GetNumberOfRows() == 0)
    {
      this->AllRawInformation->DeepCopy(this->PositionOrientationInterpreter->GetRawInformation());
      return;
    }

    vtkSmartPointer<vtkTable> raw = this->PositionOrientationInterpreter->GetRawInformation();

    for(int i = 0; i < raw->GetNumberOfRows(); i++)
    {
      vtkVariantArray * array = raw->GetRow(i);
      this->AllRawInformation->InsertNextRow(array);
    }
  }

  if(this->PositionOrientationInterpreter->HasPositionOrientationInformation())
  {
    if(this->AllPositionsOrientation->GetNumberOfPoints() == 0)
    {
      this->AllPositionsOrientation->DeepCopy(this->PositionOrientationInterpreter->GetPositionOrientation());
      return;
    }
    vtkSmartPointer<vtkPolyData> posOr = this->PositionOrientationInterpreter->GetPositionOrientation();
    for(int i = 0; i < posOr->GetNumberOfPoints(); i++)
    {
      for(int j = 0; j < posOr->GetPointData()->GetNumberOfArrays(); j++)
      {
        vtkVariant variantValue = posOr->GetPointData()->GetArray(j)->GetVariantValue(i);
        this->AllPositionsOrientation->GetPointData()->GetArray(j)->InsertVariantValue(i,variantValue);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPositionOrientationStream::ClearAllDataAvailable()
{
  this->PositionOrientationInterpreter->ResetCurrentData();
}

//----------------------------------------------------------------------------
int vtkPositionOrientationStream::CheckForNewData()
{
  return this->CheckNewDataPositionOrientation() + this->CheckForNewDataRawInformation();
}

//----------------------------------------------------------------------------
int vtkPositionOrientationStream::CheckNewDataPositionOrientation()
{
  return this->AllPositionsOrientation->GetNumberOfPoints() - this->LastNumberPositionOrientationInformation;
}

//----------------------------------------------------------------------------
int vtkPositionOrientationStream::CheckForNewDataRawInformation()
{
  return this->AllRawInformation->GetNumberOfRows() - this->LastNumberRawInformation;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkInterpreter> vtkPositionOrientationStream::GetInterpreter()
{
  return this->PositionOrientationInterpreter;
}

//----------------------------------------------------------------------------
void vtkPositionOrientationStream::SetInterpreter(vtkSmartPointer<vtkInterpreter> interpreter)
{
  this->PositionOrientationInterpreter = vtkPositionOrientationPacketInterpreter::SafeDownCast(interpreter);
}

