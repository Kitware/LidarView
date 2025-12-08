/*=========================================================================

  Program: LidarView
  Module:  vtkLVBBoxDetection.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkLVBBoxDetection_H
#define vtkLVBBoxDetection_H

#include <vtkFieldData.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <Eigen/Dense>

#include "lvCommonCoreModule.h"

using Vec3 = Eigen::Vector3d;
using Vec2 = Eigen::Vector2d;

/**
 * This class represent a vtkLVBBoxDetection.
 * It can be created from
 * - A MultiBlockDataSet (of cuboids).
 * - A vtkPolyData and a vtkIntArray of ids to be part of detection.
 * - A vector of Eigen points.
 * - Manually
 */
class LVCOMMONCORE_EXPORT vtkLVBBoxDetection
{
public:
  vtkLVBBoxDetection(int id,
    const Vec3& position,
    const Vec3& size,
    double theta,
    vtkSmartPointer<vtkFieldData> fieldData = nullptr)
    : Id(id)
    , Position(position)
    , Size(size)
    , Theta(theta)
    , FieldData(fieldData)
  {
  }

  /**
   * Create a vector of detections from a MultiBlockDataSet
   * Elements of the MultiBlockDataSet must be cuboids
   */
  static std::vector<std::shared_ptr<const vtkLVBBoxDetection>> FromMultiBlockDataSet(
    vtkSmartPointer<vtkMultiBlockDataSet> dataset);

  /**
   * Create a single detection from a vector of points
   */
  static vtkLVBBoxDetection FromPoints(int detectedClusterId,
    const std::vector<Vec3>& points,
    bool computeOrientation = true);

  /**
   * Create a vector of detections from a PolyData and an array of point ids
   */
  static std::vector<std::shared_ptr<const vtkLVBBoxDetection>> FromPolyData(
    vtkSmartPointer<vtkPolyData> polydata,
    vtkSmartPointer<vtkIntArray> ids);

  /**
   * Merge together a vector of detections into a single new one
   */
  static vtkLVBBoxDetection MergeDetections(std::vector<vtkLVBBoxDetection>& detections);

  /**
   * Returns the volume of the detection
   */
  double GetVolume() const;

  /**
   * Returns a vector of 2D corners (projection along z axis)
   */
  std::vector<Vec2> GetCorners2D() const;

  /**
   * Compute the volume of the intersection between the detection and another
   */
  double GetIntersection(const vtkLVBBoxDetection& other) const;

  /**
   * Compute distance with another detection using custom coefficients
   */
  double DistanceTo(const vtkLVBBoxDetection& other,
    double iouCoef,
    double positionCoef,
    double sizeCoef,
    double thetaCoef) const;

  /**
   * Compute Intersection over Union with another detection (IoU)
   */
  double IoU(const vtkLVBBoxDetection& other) const;

  /**
   * Return the intersection volume with 'other' detection divided by self volume.
   */
  double IoS(const vtkLVBBoxDetection& other) const;

  /**
   * Create a cuboid PolyData from current detection
   */
  vtkSmartPointer<vtkPolyData> GetBoundingBoxPolyData() const;

  /**
   * Return the Id of the detection
   */
  int GetId() const { return Id; }

  /**
   * Return the position of the detection
   */
  const Vec3& GetPosition() const { return Position; }

  /**
   * Return the size of the detection
   */
  const Vec3& GetSize() const { return Size; }

  /**
   * Return the orientation of the bounding box arround z-axis (in radians)
   */
  double GetTheta() const { return Theta; }

  /**
   * Return field data stored by this detection
   */
  vtkSmartPointer<vtkFieldData> GetFieldData() const { return FieldData; }

  /**
   * Replace field data of the detection by a new one
   */
  void SetFieldData(vtkSmartPointer<vtkFieldData> fd) { this->FieldData = fd; }

private:
  int Id;
  Vec3 Position;
  Vec3 Size;
  double Theta;
  vtkSmartPointer<vtkFieldData> FieldData;
};

using SharedBBoxDetection = std::shared_ptr<const vtkLVBBoxDetection>;

#endif