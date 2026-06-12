// This file is needed to the python wrapping for vtk :
// The CMake mechanism of VTK needs it to generate some dependency graphs

#include "vtkInterpreter.h"

#include <vtkCommand.h>
#include <vtkTransform.h>

#include <algorithm>

//-----------------------------------------------------------------------------
vtkInterpreter::~vtkInterpreter()
{
  this->SetSensorTransform(nullptr);
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkInterpreter::GetMTime()
{
  if (this->SensorTransform)
  {
    return std::max(this->Superclass::GetMTime(), this->SensorTransform->GetMTime());
  }
  return this->Superclass::GetMTime();
}

//-----------------------------------------------------------------------------
void vtkInterpreter::SetSensorTransform(vtkTransform* interpreter)
{
  if (this->SensorTransform == interpreter)
  {
    return;
  }

  if (this->SensorTransform)
  {
    this->SensorTransform->RemoveObserver(this->TransformObserverId);
  }
  vtkSetObjectBodyMacro(SensorTransform, vtkTransform, interpreter);
  if (this->SensorTransform)
  {
    this->TransformObserverId = this->SensorTransform->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkInterpreter::Modified);
  }
}
