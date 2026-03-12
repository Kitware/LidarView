/*=========================================================================

  Program: LidarView
  Module:  vtkLiveSourceAlgorithm.txx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLiveSourceAlgorithm.h"

#include <vtkDemandDrivenPipeline.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>

//------------------------------------------------------------------------------
template <class AlgorithmT>
void vtkLiveSourceAlgorithm<AlgorithmT>::UpdateLiveSource()
{
  this->Superclass::Modified();
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
void vtkLiveSourceAlgorithm<AlgorithmT>::Modified()
{
  this->NeedInitialization = true;
  this->Superclass::Modified();
}

//------------------------------------------------------------------------------
template <class AlgorithmT>
vtkTypeBool vtkLiveSourceAlgorithm<AlgorithmT>::ProcessRequest(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    if (!this->NeedInitialization)
    {
      return 1;
    }
    this->NeedInitialization = false;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
