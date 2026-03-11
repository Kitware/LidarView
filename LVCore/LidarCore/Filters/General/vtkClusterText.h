/*=========================================================================

  Program: LidarView
  Module:  vtkClusterText.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkClusterText_h
#define vtkClusterText_h

// vtk includes
#include <vtkMultiBlockDataSetAlgorithm.h>
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersGeneralModule.h"

/**
 * @class vtkClusterText
 * @brief Show a cluster information around
 */
class LVFILTERSGENERAL_EXPORT vtkClusterText : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkClusterText* New();
  vtkTypeMacro(vtkClusterText, vtkMultiBlockDataSetAlgorithm);

  // Setter/getter
  // Array name of the cluster information to display
  vtkSetStringMacro(FieldDataArrayName);
  vtkGetStringMacro(FieldDataArrayName);

  // Array name of the cluster information to display
  vtkSetMacro(TextScale, double);
  vtkGetMacro(TextScale, double);

protected:
  vtkClusterText() = default;
  ~vtkClusterText() = default;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkClusterText(const vtkClusterText&) = delete;
  void operator=(const vtkClusterText&) = delete;

  // Array name of the cluster information to display
  char* FieldDataArrayName = nullptr;

  // Parameter to change the text size
  double TextScale = 0.8;
};

#endif // vtkClusterText_h
