/*=========================================================================

  Program: LidarView
  Module:  vtkProcessingSample.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkProcessingSample_h
#define __vtkProcessingSample_h

#include <vtkPolyDataAlgorithm.h>

#include "ProcessingSampleModule.h"

class PROCESSINGSAMPLE_EXPORT vtkProcessingSample : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkProcessingSample, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkProcessingSample* New();

protected:
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  vtkProcessingSample();
  virtual ~vtkProcessingSample();

private:
  vtkProcessingSample(const vtkProcessingSample&); // Not implemented.
  void operator=(const vtkProcessingSample&);      // Not implemented.
};

#endif
