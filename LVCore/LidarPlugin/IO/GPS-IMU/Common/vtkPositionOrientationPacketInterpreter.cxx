#include "vtkPositionOrientationPacketInterpreter.h"

//------------------------------------------------------------------------------
void vtkPositionOrientationPacketInterpreter::ResetCurrentData()
{
  if(this->HasPositionOrientationInformation())
  {
    this->PositionOrientation = vtkSmartPointer<vtkPolyData>::New();
  }

  if(this->HasRawInformation())
  {
    this->RawInformation = vtkTable::New();
  }
  this->InitArrays();
}

//------------------------------------------------------------------------------
bool vtkPositionOrientationPacketInterpreter::IsNewRawInformation()
{
  if(!this->HasRawInformation())
  {
    return false;
  }
  bool isNewRaw = this->SizeRawInformationLastAsk != this->RawInformation->GetNumberOfRows();
  this->SizeRawInformationLastAsk = this->RawInformation->GetNumberOfRows();
  return isNewRaw;
}

//------------------------------------------------------------------------------
bool vtkPositionOrientationPacketInterpreter::IsNewPositionOrientationInformation()
{
  if(!this->HasPositionOrientationInformation())
  {
    return false;
  }
  bool isNewPos = this->SizePositionOrientationInformationLastAsk != this->PositionOrientation->GetNumberOfPoints();
  this->SizePositionOrientationInformationLastAsk = this->PositionOrientation->GetNumberOfPoints();
  return isNewPos;
}

//------------------------------------------------------------------------------
bool vtkPositionOrientationPacketInterpreter::IsNewData()
{
   return (this->IsNewRawInformation() || this->IsNewPositionOrientationInformation());
}

