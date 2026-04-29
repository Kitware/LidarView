/*=========================================================================

  Program: LidarView
  Module:  vtkINSTableToPoses.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkINSTableToPoses_h
#define vtkINSTableToPoses_h

#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersGeneralModule.h"

/**
 * Convert a vtkTable of INS data (read with vtkNormalizeExternalSensorData)
 * to a vtkPolyData of the same format of vtkTemporalTransformReader.
 */
class LVFILTERSGENERAL_EXPORT vtkINSTableToPoses : public vtkPolyDataAlgorithm
{
public:
  static vtkINSTableToPoses* New();
  vtkTypeMacro(vtkINSTableToPoses, vtkPolyDataAlgorithm);

protected:
  vtkINSTableToPoses() = default;
  ~vtkINSTableToPoses() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkINSTableToPoses(const vtkINSTableToPoses&) = delete;
  void operator=(const vtkINSTableToPoses&) = delete;
};

#endif // vtkINSTableToPoses_h
