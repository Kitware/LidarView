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


#ifndef VTK_PLANE_PROJECTOR_H
#define VTK_PLANE_PROJECTOR_H

// VTK
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

// EIGEN
#include <Eigen/Dense>

#include "LidarCoreModule.h"

/**
 * @brief vtkPlaneProjector Projects a 3d point cloud onto a plane.
 *        It returns:
 *        - a 3D point cloud with points aligned on the provided input plane.
 *        - the same point cloud rotated so that it matches the XY plane (which
 *        can be later transformed in a 2D vtkImageData)
 *        The plane to project on can either be user defined or passed as input
 */

class LIDARCORE_EXPORT vtkPlaneProjector : public vtkPolyDataAlgorithm
{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  static vtkPlaneProjector *New();
  vtkTypeMacro(vtkPlaneProjector, vtkPolyDataAlgorithm)

  vtkSetMacro(UsePlaneFromInput, bool);

  // Set the projection plane normal
  void SetPlaneNormal(double w0, double w1, double w2)
  {
    this->Normal << w0, w1, w2;
    this->Modified();
  };

  // Set the projection plane
  void SetPlaneOrigin(double w0, double w1, double w2)
  {
    this->Origin << w0, w1, w2;
    this->Modified();
  };

protected:
  vtkPlaneProjector();
  ~vtkPlaneProjector() = default;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

private:
  vtkPlaneProjector(const vtkPlaneProjector&) = delete;
  void operator=(const vtkPlaneProjector&) = delete;

  // If true: the user defined plane is ignored and the Origin/Normal are
  // defined from the PolyData provided in PLANE_INPUT_PORT. It must contain at
  // least one point (the first point coordinates is used as Origin) and have a
  // "Normals" PointData array (its value for the first point is used as Normal)
  bool UsePlaneFromInput = false;

  Eigen::Vector3d Normal = Eigen::Vector3d::UnitZ();  // project on XY plane
  Eigen::Vector3d Origin = Eigen::Vector3d::Zero();
};

#endif // VTK_PLANE_PROJECTOR_H
