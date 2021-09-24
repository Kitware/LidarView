// This file is needed to the python wrapping for vtk :
// The CMake mechanism of VTK needs it to generate some dependency graphs

#include <vtkTransform.h>
#include <algorithm>

#include "IO/vtkInterpreter.h"

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkInterpreter, SensorTransform, vtkTransform)

//-----------------------------------------------------------------------------
vtkMTimeType vtkInterpreter::GetMTime()
{
  if (this->SensorTransform)
  {
    return std::max(this->Superclass::GetMTime(), this->SensorTransform->GetMTime());
  }
  return this->Superclass::GetMTime();
}
