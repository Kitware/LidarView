/*=========================================================================

  Program:   LidarView
  Module:    vtkCurbDetectorOffline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCurbDetectorOffline.h"

#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <Eigen/Dense>

vtkStandardNewMacro(vtkCurbDetectorOffline);

//-----------------------------------------------------------------------------
vtkCurbDetectorOffline::vtkCurbDetectorOffline()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
int vtkCurbDetectorOffline::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCurbDetectorOffline::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPolyData* trajectory = vtkPolyData::GetData(inputVector[1], 0);

  vtkPolyData* planesOut = vtkPolyData::GetData(outputVector, 0);

  if (trajectory == nullptr || trajectory->GetPoints() == nullptr ||
    trajectory->GetNumberOfPoints() < 2)
  {
    vtkErrorMacro("Invalid trajectory: need at least 2 poses");
    return 0;
  }

  vtkIdType npts = trajectory->GetNumberOfPoints();

  if (this->SliceLeftDistance < 0.0 || this->SliceRightDistance < 0.0)
  {
    vtkErrorMacro("SliceLeftDistance and SliceRightDistance must be non-negative");
    return 0;
  }

  double lateralMin = -this->SliceLeftDistance;
  double lateralMax = this->SliceRightDistance;
  double verticalMin = this->SliceDownwardDistance;
  double verticalMax = this->SliceUpwardDistance;

  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  vtkNew<vtkCellArray> polys;

  // Keep plane parameters for slicing aggregated cloud later
  std::vector<Eigen::Vector3d> planeCenters;
  std::vector<Eigen::Vector3d> planeNormals;
  std::vector<Eigen::Vector3d> planeL;
  std::vector<Eigen::Vector3d> planeV;

  // Use all available consecutive segments along trajectory
  const int maxSegments = static_cast<int>(std::max<vtkIdType>(0, npts - 1));
  planeCenters.reserve(maxSegments);
  planeNormals.reserve(maxSegments);
  planeL.reserve(maxSegments);
  planeV.reserve(maxSegments);

  for (int i = 0; i < maxSegments; ++i)
  {
    Eigen::Vector3d p0, p1;
    trajectory->GetPoints()->GetPoint(i, p0.data());
    trajectory->GetPoints()->GetPoint(i + 1, p1.data());

    // Center of the current slice, defined as the midpoint of the trajectory segment.
    const Eigen::Vector3d sliceCenter = 0.5 * (p0 + p1);

    // Local slice normal aligned with the trajectory direction.
    Eigen::Vector3d sliceNormal = (p1 - p0).normalized();

    // Build a local slice frame:
    //  - vertical direction derived from world-up,
    //  - lateral direction perpendicular to both the trajectory and the vertical.
    const Eigen::Vector3d worldUp = Eigen::Vector3d::UnitZ();
    Eigen::Vector3d axisVertical = worldUp - worldUp.dot(sliceNormal) * sliceNormal;

    // Normalize the slice directions and orient the lateral axis so that positive values
    // correspond to the physical right side of the trajectory.
    if (axisVertical.norm() > 0.0)
    {
      axisVertical.normalize();
    }
    Eigen::Vector3d axisLateral = sliceNormal.cross(axisVertical);
    if (axisLateral.norm() > 0.0)
    {
      axisLateral.normalize();
    }

    // Build a rectangular slicing window around the trajectory using the lateral
    // and vertical limits provided by the user.
    vtkIdType baseId = pts->GetNumberOfPoints();
    // Center the slicing window vertically according to the configured bounds.
    const double verticalCenter = 0.5 * (verticalMin + verticalMax);
    const double halfVerticalSpan = 0.5 * (verticalMax - verticalMin);

    const Eigen::Vector3d basePoint = sliceCenter + verticalCenter * axisVertical;

    // Compute the four rectangle corners: left/right and lower/upper bounds.
    const Eigen::Vector3d cornerLeftLower =
      basePoint + lateralMin * axisLateral - halfVerticalSpan * axisVertical;

    const Eigen::Vector3d cornerRightLower =
      basePoint + lateralMax * axisLateral - halfVerticalSpan * axisVertical;

    const Eigen::Vector3d cornerRightUpper =
      basePoint + lateralMax * axisLateral + halfVerticalSpan * axisVertical;

    const Eigen::Vector3d cornerLeftUpper =
      basePoint + lateralMin * axisLateral + halfVerticalSpan * axisVertical;

    pts->InsertNextPoint(cornerLeftLower.data());
    pts->InsertNextPoint(cornerRightLower.data());
    pts->InsertNextPoint(cornerRightUpper.data());
    pts->InsertNextPoint(cornerLeftUpper.data());

    vtkIdType ids[4] = { baseId + 0, baseId + 1, baseId + 2, baseId + 3 };
    polys->InsertNextCell(4, ids);

    // Store slice geometry for later point projection and profile extraction.
    planeCenters.push_back(sliceCenter);
    planeNormals.push_back(sliceNormal);
    planeL.push_back(axisLateral);
    planeV.push_back(axisVertical);
  }

  if (planesOut)
  {
    planesOut->SetPoints(pts);
    planesOut->SetPolys(polys);
  }

  return 1;
}
