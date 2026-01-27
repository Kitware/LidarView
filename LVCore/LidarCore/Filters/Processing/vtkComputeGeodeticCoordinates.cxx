/*=========================================================================

  Program: LidarView
  Module:  vtkComputeGeodeticCoordinates.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkComputeGeodeticCoordinates.h"

#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtk_libproj.h>

vtkStandardNewMacro(vtkComputeGeodeticCoordinates)

// ----------------------------------------------------------------------------
int vtkComputeGeodeticCoordinates::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0]->GetInformationObject(0));
  vtkPolyData* output = vtkPolyData::GetData(outputVector->GetInformationObject(0));

  if (!input || !output)
  {
    vtkErrorMacro("Invalid input or output");
    return 0;
  }

  vtkPoints* pts = input->GetPoints();
  if (!pts)
  {
    vtkErrorMacro("Input has no points");
    return 0;
  }

  output->ShallowCopy(input);

  const std::string& inCRS = this->InputCRS;
  const std::string& outCRS = this->OutputCRS;

  if (inCRS.empty() || outCRS.empty())
  {
    vtkErrorMacro("CRS strings are empty. Please set both InputCRS and OutputCRS.");
    return 0;
  }

  // Initialize PROJ conversion
  PJ_CONTEXT* pjContext = proj_context_create();
  if (!pjContext)
  {
    vtkErrorMacro("Failed to create PROJ context");
    return 0;
  }

  PJ* pipeline = proj_create_crs_to_crs(pjContext, inCRS.c_str(), outCRS.c_str(), nullptr);
  if (!pipeline)
  {
    vtkErrorMacro(<< "Unable to create transformation from '" << inCRS << "' to '" << outCRS
                  << "'");
    proj_context_destroy(pjContext);
    return 0;
  }

  // Normalize for visualization to enforce axis order as lon,lat[,height]
  PJ* visPipeline = proj_normalize_for_visualization(pjContext, pipeline);
  proj_destroy(pipeline);

  vtkIdType npts = pts->GetNumberOfPoints();
  vtkNew<vtkDoubleArray> lonArr;
  vtkNew<vtkDoubleArray> latArr;
  vtkNew<vtkDoubleArray> altArr;
  lonArr->SetName("Longitude");
  latArr->SetName("Latitude");
  altArr->SetName("Altitude");
  lonArr->SetNumberOfTuples(npts);
  latArr->SetNumberOfTuples(npts);
  altArr->SetNumberOfTuples(npts);

  // Write geographic coordinates
  double xyz[3];
  for (vtkIdType i = 0; i < npts; ++i)
  {
    pts->GetPoint(i, xyz);
    PJ_COORD inCoord = proj_coord(xyz[0], xyz[1], xyz[2], 0);
    PJ_COORD outCoord = proj_trans(visPipeline, PJ_FWD, inCoord);

    const double lon = outCoord.lpz.lam;
    const double lat = outCoord.lpz.phi;
    const double alt = outCoord.lpz.z;

    lonArr->SetValue(i, lon);
    latArr->SetValue(i, lat);
    altArr->SetValue(i, alt);
  }

  output->GetPointData()->AddArray(lonArr);
  output->GetPointData()->AddArray(latArr);
  output->GetPointData()->AddArray(altArr);

  proj_destroy(visPipeline);
  proj_context_destroy(pjContext);
  return 1;
}
