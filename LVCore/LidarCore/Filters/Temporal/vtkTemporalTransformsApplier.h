/*=========================================================================

  Program:   LidarView
  Module:    vtkTemporalTransformApplier.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkTemporalTransformApplier_h
#define vtkTemporalTransformApplier_h

#include <vtkNew.h>
#include <vtkPolyDataAlgorithm.h>

#include "vtkCustomTransformInterpolator.h"

#include "lvFiltersTemporalModule.h"

/**
 * @brief The vtkTemporalTransformsApplier take 2 inputs : a vtkTemporalTransforms which
 * contains the poses and orientation of the sensor and a polydata.
 * The filter will apply the transform corresponding to the pipeline time or to the current
 * pointcloud time. This Option can be manage with the InterpolateEachPoint paramater
 */
class LVFILTERSTEMPORAL_EXPORT vtkTemporalTransformsApplier : public vtkPolyDataAlgorithm
{
public:
  static vtkTemporalTransformsApplier* New();
  vtkTypeMacro(vtkTemporalTransformsApplier, vtkPolyDataAlgorithm);

  /**
   * Override GetMTime() because we depend on the TransformInterpolator
   * which may be modified outside of this class.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Interpolator used to get the right transform
   */
  int GetInterpolationType() { return this->Interpolator->GetInterpolationType(); }
  void SetInterpolationType(int value) { this->Interpolator->SetInterpolationType(value); };
  //@}

  //@{
  /**
   * Indicate if a different transform should be apply to each point,
   * or if the same transform should be apply to the whole point cloud.
   * In the first case you must specify the array from the pointcloud containing the
   * timestamp with 'SetInputArrayToProcess'
   */
  vtkGetMacro(InterpolateEachPoint, bool);
  vtkSetMacro(InterpolateEachPoint, bool);
  //@}

  //@{
  /**
   * Convert time in second default is for data in microsecond
   */
  vtkGetMacro(ConversionFactorToSecond, double);
  vtkSetMacro(ConversionFactorToSecond, double);
  //@}

protected:
  vtkTemporalTransformsApplier();

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  bool InterpolateEachPoint;
  vtkSmartPointer<vtkCustomTransformInterpolator> Interpolator;
  double ConversionFactorToSecond = 1e-6;

  vtkTemporalTransformsApplier(const vtkTemporalTransformsApplier&) /*= delete*/;
  void operator=(const vtkTemporalTransformsApplier&) /*= delete*/;
};

#endif // vtkTemporalTransformApplier_h
