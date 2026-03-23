/*=========================================================================

  Program: LidarView
  Module:  vtkComputeNormalAdaptive.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Local includes
#include "vtkComputeNormalAdaptive.h"
#include "NormalsEstimator.h"

// VTK includes
#include <vtkInformation.h>

vtkStandardNewMacro(vtkComputeNormalAdaptive)

//-----------------------------------------------------------------------------
int vtkComputeNormalAdaptive::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* pointCloud = vtkPolyData::GetData(inputVector[0], 0);
  vtkPolyData* pointCloudWithNormals = vtkPolyData::GetData(outputVector, 0);

  if (this->SearchMode == 0)
  {
    // Compute normals using knn as search mode
    NormalEstimation::ComputeNormalsKnn(pointCloud,
      pointCloudWithNormals,
      this->NumberNeighbors,
      this->Noise,
      this->Radius,
      this->Orient,
      this->Denoise);
  }
  else
  {
    // Compute normals using radius as search mode
    NormalEstimation::ComputeNormalsRadius(pointCloud,
      pointCloudWithNormals,
      this->Radius,
      this->Noise,
      this->NumberNeighbors,
      this->Orient,
      this->Denoise);
  }

  return 1;
}
