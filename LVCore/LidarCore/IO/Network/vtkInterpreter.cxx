// This file is needed to the python wrapping for vtk :
// The CMake mechanism of VTK needs it to generate some dependency graphs

#include "vtkInterpreter.h"

#include <vtkTransform.h>

#include <algorithm>

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkInterpreter, SensorTransform, vtkTransform);

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
