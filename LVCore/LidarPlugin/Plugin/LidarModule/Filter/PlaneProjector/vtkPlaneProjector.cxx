//=========================================================================
//
// Copyright 2020 Kitware, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

// LOCAL
#include "vtkPlaneProjector.h"

// VTK
#include <vtkInformation.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkTransform.h>
#include <vtkNew.h>
#include <vtkMath.h>

constexpr unsigned int POINTS_INPUT_PORT = 0;
constexpr unsigned int PLANE_INPUT_PORT = 1;
constexpr unsigned int INPUT_PORT_COUNT = 2;

constexpr unsigned int RAW_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int XY_POINTS_OUTPUT_PORT = 1;
constexpr unsigned int OUTPUT_PORT_COUNT = 2;


vtkStandardNewMacro(vtkPlaneProjector)

//-----------------------------------------------------------------------------
vtkPlaneProjector::vtkPlaneProjector()
{
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//-----------------------------------------------------------------------------
int vtkPlaneProjector::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == POINTS_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    return 1;
  }
  if (port == PLANE_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData" );
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkPlaneProjector::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get IO
  vtkPolyData* inCloud = vtkPolyData::GetData(inputVector[POINTS_INPUT_PORT], 0);
  vtkPolyData* inPlane = vtkPolyData::GetData(inputVector[PLANE_INPUT_PORT], 0);
  vtkPolyData* rawOutCloud = vtkPolyData::GetData(outputVector, RAW_POINTS_OUTPUT_PORT);
  vtkPolyData* xyOutCloud = vtkPolyData::GetData(outputVector, XY_POINTS_OUTPUT_PORT);

  rawOutCloud->DeepCopy(inCloud);
  xyOutCloud->DeepCopy(inCloud);

  Eigen::Vector3d normal;
  Eigen::Vector3d origin;
  bool planeFromInputOk = false;
  if (this->UsePlaneFromInput)
  {
    if (inPlane != nullptr)
    {
      if ((inPlane->GetNumberOfPoints() > 0) &&
          (inPlane->GetPointData()->GetArray("Normals") != nullptr))
      {
        double* pt = inPlane->GetPoint(0);
        origin << pt[0], pt[1], pt[2];
        double* n = inPlane->GetPointData()->GetArray("Normals")->GetTuple3(0);
        normal << n[0], n[1], n[2];
        planeFromInputOk = true;
      }
    }
    if (!planeFromInputOk)
    {
      vtkWarningMacro("Could not get plane from input, using user provided plane instead");
    }
  }

  if (!planeFromInputOk)
  {
    normal = this->Normal;
    origin = this->Origin;
  }

  Eigen::Vector3d unitNormal = normal.normalized();

  // Compute the transform between the projected plane and XY plane
  Eigen::Quaterniond rotation = Eigen::Quaterniond::FromTwoVectors(unitNormal, Eigen::Vector3d::UnitZ());

  for (vtkIdType pointIndex = 0; pointIndex < inCloud->GetNumberOfPoints(); ++pointIndex)
  {
    double* pt = inCloud->GetPoint(pointIndex);
    Eigen::Vector3d X(pt[0], pt[1], pt[2]);
    double dist = (X - origin).dot(unitNormal);
    Eigen::Vector3d y = X - dist * unitNormal;
    Eigen::Vector3d yrot = rotation * y;
    pt[0] = y.x();
    pt[1] = y.y();
    pt[2] = y.z();
    rawOutCloud->GetPoints()->SetPoint(pointIndex, pt);

    pt[0] = yrot.x();
    pt[1] = yrot.y();
    pt[2] = 0;  // yrot.z();
    xyOutCloud->GetPoints()->SetPoint(pointIndex, pt);
  }

  return 1;
}
