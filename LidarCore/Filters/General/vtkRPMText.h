/*=========================================================================

  Program: LidarView
  Module:  vtkRPMText.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkRPMText_h
#define vtkRPMText_h

// vtk includes
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkTableAlgorithm.h>

#include "lvFiltersGeneralModule.h"

#include <string>

/**
 * @class vtkRPMText
 * @brief Create and show a Text with RPM info
 */
class LVFILTERSGENERAL_EXPORT vtkRPMText : public vtkTableAlgorithm
{
public:
  static vtkRPMText* New();
  vtkTypeMacro(vtkRPMText, vtkTableAlgorithm);

protected:
  vtkRPMText();
  ~vtkRPMText() = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkRPMText(const vtkRPMText&) = delete;
  void operator=(const vtkRPMText&) = delete;
};

#endif // vtkRPMText_h
