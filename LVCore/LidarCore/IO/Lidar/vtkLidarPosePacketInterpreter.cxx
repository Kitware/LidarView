#include "vtkLidarPosePacketInterpreter.h"

//------------------------------------------------------------------------------
void vtkLidarPosePacketInterpreter::ResetCurrentData()
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
bool vtkLidarPosePacketInterpreter::IsNewRawInformation()
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
bool vtkLidarPosePacketInterpreter::IsNewPositionOrientationInformation()
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
bool vtkLidarPosePacketInterpreter::IsNewData()
{
   return (this->IsNewRawInformation() || this->IsNewPositionOrientationInformation());
}

