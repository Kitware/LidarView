/*=========================================================================

  Program: LidarView
  Module:  vtkLVBBoxDetection.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <limits>

#include <vtkCleanPolyData.h>
#include <vtkCubeSource.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include "vtkLVBBoxDetection.h"

//------------------------------------------------------------------------------
bool isOrthogonal(const Vec3& v1, const Vec3& v2)
{
  double dot = v1.x() * v2.x() + v1.y() * v2.y() + v1.z() * v2.z();
  double mag = v1.norm() * v2.norm();

  if (mag < 1e-6)
  {
    return false;
  }

  return std::abs(dot / mag) < 1e-3;
}

//------------------------------------------------------------------------------
std::vector<SharedBBoxDetection> vtkLVBBoxDetection::FromMultiBlockDataSet(
  vtkSmartPointer<vtkMultiBlockDataSet> dataset)
{
  auto detections = std::vector<SharedBBoxDetection>();
  for (unsigned int i = 0; i < dataset->GetNumberOfBlocks(); ++i)
  {
    auto block = vtkPolyData::SafeDownCast(dataset->GetBlock(i));
    if (!block || block->GetNumberOfPoints() != 8)
    {
      continue;
    }

    auto points = block->GetPoints();

    // Compute bbox center
    Vec3 center(0, 0, 0);
    for (int j = 0; j < 8; ++j)
    {
      center += Vec3(points->GetPoint(j));
    }
    center /= 8.0;

    // We define P0 as base point (can be any point)
    Vec3 P0(points->GetPoint(0));
    std::vector<Vec3> candidates;
    for (int j = 1; j < 8; ++j)
    {
      candidates.push_back(Vec3(points->GetPoint(j)) - P0);
    }

    // We search for 3 othogonal edges
    // Va . Vb == 0 && Va . Vc == 0 && Vb . Vc == 0
    Vec3 edges[3];
    bool found = false;
    for (int a = 0; a < 7 && !found; ++a)
    {
      for (int b = a + 1; b < 7 && !found; ++b)
      {
        if (isOrthogonal(candidates[a], candidates[b]))
        {
          for (int c = b + 1; c < 7 && !found; ++c)
          {
            if (isOrthogonal(candidates[a], candidates[c]) &&
              isOrthogonal(candidates[b], candidates[c]))
            {
              edges[0] = candidates[a];
              edges[1] = candidates[b];
              edges[2] = candidates[c];
              found = true;
            }
          }
        }
      }
    }

    // From our 3 candidates, we define the zone as the one which is the most orthogonal to z
    int zIdx = 0;
    double maxZAlignment = -1.0;
    for (int j = 0; j < 3; ++j)
    {
      double alignment = std::abs(edges[j].z() / edges[j].norm());
      if (alignment > maxZAlignment)
      {
        maxZAlignment = alignment;
        zIdx = j;
      }
    }

    Vec3 edgeZ = edges[zIdx];
    Vec3 edgeX = edges[(zIdx + 1) % 3];
    Vec3 edgeY = edges[(zIdx + 2) % 3];

    double sizeX = edgeX.norm();
    double sizeY = edgeY.norm();
    double sizeZ = edgeZ.norm();

    // Rotation angle of bounding box around axisZ from axisX
    double theta = std::atan2(edgeX.y(), edgeX.x());

    // We want to priorize the biggest dimension for x axis.
    // This way, rectangular detections (such as car), will always orient the same way and will not
    // disturb tracking algorithms.
    // It helps to have a continuity across detections from frame to frame:
    // E.g. Size(2, 4), theta(0) == Size(4, 2), theta(pi/2), so we want to normalize it.
    // Moreover, it is easier to filter detections by size (if we know target size):
    // E.g. Cars will have a length between 3 and 5 meters, and a width of 2 to 3 meters.
    if (sizeY > sizeX)
    {
      std::swap(edgeX, edgeY);
      std::swap(sizeX, sizeY);
      theta += vtkMath::Pi() / 2;
    }
    Vec3 size(sizeX, sizeY, sizeZ);

    while (theta > vtkMath::Pi() / 2)
    {
      theta -= vtkMath::Pi();
    }

    while (theta < -vtkMath::Pi() / 2)
    {
      theta += vtkMath::Pi();
    }

    auto fd = vtkSmartPointer<vtkFieldData>::New();
    fd->DeepCopy(block->GetFieldData());
    auto detection =
      std::make_shared<vtkLVBBoxDetection>(vtkLVBBoxDetection(i, center, size, theta, fd));
    detections.emplace_back(detection);
  }

  return detections;
}

//------------------------------------------------------------------------------
vtkLVBBoxDetection vtkLVBBoxDetection::FromPoints(int detectedClusterId,
  const std::vector<Vec3>& points,
  bool computeOrientation)
{
  double theta = 0;
  double minX = std::numeric_limits<double>::max();
  double maxX = std::numeric_limits<double>::min();
  double minY = std::numeric_limits<double>::max();
  double maxY = std::numeric_limits<double>::min();
  double minZ = std::numeric_limits<double>::max();
  double maxZ = std::numeric_limits<double>::min();
  for (const auto& p : points)
  {
    minZ = std::min(minZ, p.z());
    maxZ = std::max(maxZ, p.z());
  }

  double xSize = maxX - minX;
  double ySize = maxY - minY;
  double zSize = maxZ - minZ;

  Vec3 centroid = Vec3((minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2);

  std::vector<Vec3> centeredPoints;
  for (const auto& p : points)
  {
    centeredPoints.push_back(p - centroid);
  }

  Vec3 rotatedCentroid = Vec3::Zero();

  // Compute z-orientation of given points set:
  // We compute the axis-aligned englobbing bounding box for many rotations between 0 and Pi
  // Then, we select the bbox with the lowest area
  if (computeOrientation)
  {
    double lowestSurface = std::numeric_limits<double>::max();
    int tries = 40;
    for (int i = 0; i < tries; ++i)
    {
      double t = -vtkMath::Pi() / tries * i;
      Eigen::Matrix3d rz = Eigen::AngleAxisd(t, Vec3::UnitZ()).toRotationMatrix();

      std::vector<Vec3> newRelPos;
      for (const auto& p : centeredPoints)
      {
        newRelPos.push_back(rz * p);
      }

      double minx = std::numeric_limits<double>::max();
      double maxx = std::numeric_limits<double>::min();
      double miny = std::numeric_limits<double>::max();
      double maxy = std::numeric_limits<double>::min();
      for (const auto& p : newRelPos)
      {
        minx = std::min(minx, p.x());
        maxx = std::max(maxx, p.x());
        miny = std::min(miny, p.y());
        maxy = std::max(maxy, p.y());
      }

      double surface = (maxx - minx) * (maxy - miny);
      if (surface < lowestSurface)
      {
        theta = -t;
        lowestSurface = surface;
        Eigen::Matrix3d rzInverted = Eigen::AngleAxisd(-t, Vec3::UnitZ()).toRotationMatrix();

        double minDx = std::numeric_limits<double>::max();
        double maxDx = std::numeric_limits<double>::min();
        double minDy = std::numeric_limits<double>::max();
        double maxDy = std::numeric_limits<double>::min();
        for (const auto& p : newRelPos)
        {
          minDx = std::min(minDx, p.x());
          maxDx = std::max(maxDx, p.x());
          minDy = std::min(minDy, p.y());
          maxDy = std::max(maxDy, p.y());
        }
        Vec3 centroidDelta = Vec3((minDx + maxDx) / 2, (minDy + maxDy) / 2, 0);
        rotatedCentroid = rzInverted * centroidDelta;

        xSize = maxx - minx;
        ySize = maxy - miny;
        if (xSize < ySize)
        {
          std::swap(xSize, ySize);
          theta += vtkMath::Pi() / 2;
        }
      }
    }
    centroid += rotatedCentroid;
  }
  else
  {
    double minx = std::numeric_limits<double>::max();
    double maxx = std::numeric_limits<double>::min();
    double miny = std::numeric_limits<double>::max();
    double maxy = std::numeric_limits<double>::min();
    for (const auto& p : points)
    {
      minx = std::min(minx, p.x());
      maxx = std::max(maxx, p.x());
      miny = std::min(miny, p.y());
      maxy = std::max(maxy, p.y());
    }

    xSize = maxx - minx;
    ySize = maxy - miny;
    if (xSize < ySize)
    {
      std::swap(xSize, ySize);
      theta += vtkMath::Pi() / 2;
    }
  }

  while (theta > vtkMath::Pi() / 2)
  {
    theta -= vtkMath::Pi();
  }

  while (theta < -vtkMath::Pi() / 2)
  {
    theta += vtkMath::Pi();
  }

  return vtkLVBBoxDetection(detectedClusterId, centroid, Vec3(xSize, ySize, zSize), theta);
}

//------------------------------------------------------------------------------
std::vector<std::shared_ptr<const vtkLVBBoxDetection>> vtkLVBBoxDetection::FromPolyData(
  vtkSmartPointer<vtkPolyData> polydata,
  vtkSmartPointer<vtkIntArray> ids)
{
  std::vector<std::shared_ptr<const vtkLVBBoxDetection>> detections;
  std::unordered_map<int, std::vector<int>> clusters;

  for (int i = 0; i < ids->GetNumberOfValues(); i++)
  {
    int id = ids->GetValue(i);
    clusters[id].push_back(i);
  }

  for (const auto& [id, pointIndices] : clusters)
  {
    std::vector<Vec3> points;
    for (vtkIdType idx : pointIndices)
    {
      double* pt = polydata->GetPoint(idx);
      points.push_back(Vec3(pt));
    }

    auto detection = vtkLVBBoxDetection::FromPoints(id, points);
    detections.push_back(std::make_shared<vtkLVBBoxDetection>(detection));
  }

  return detections;
}

//------------------------------------------------------------------------------
vtkLVBBoxDetection vtkLVBBoxDetection::MergeDetections(std::vector<vtkLVBBoxDetection>& detections)
{
  double minVal = std::numeric_limits<double>::min();
  double maxVal = std::numeric_limits<double>::max();
  double mins[3] = { maxVal, maxVal, maxVal };
  double maxs[3] = { minVal, minVal, minVal };

  for (auto detection : detections)
  {
    for (int i = 0; i < 3; i++)
    {
      mins[i] = std::min(detection.GetPosition()(i) - detection.GetSize()(i) / 2, mins[i]);
      maxs[i] = std::max(detection.GetPosition()(i) + detection.GetSize()(i) / 2, maxs[i]);
    }
  }

  Vec3 minPos = Vec3(mins);
  Vec3 maxPos = Vec3(maxs);
  Vec3 size = maxPos - minPos;
  Vec3 center = minPos + size / 2;

  // When merging bboxes, we currently set theta to 0
  return vtkLVBBoxDetection(-1, center, size, 0);
}

//------------------------------------------------------------------------------
double vtkLVBBoxDetection::DistanceTo(const vtkLVBBoxDetection& other,
  double iouCoef,
  double positionCoef,
  double sizeCoef,
  double thetaCoef) const
{
  const double posDist = (Position - other.GetPosition()).norm();
  const double sizeDist = (Size - other.GetSize()).norm();

  double thetaDifference = std::abs(GetTheta() - other.GetTheta());
  double orientationDist = std::min(thetaDifference, std::abs(vtkMath::Pi() / 2 - thetaDifference));

  return iouCoef * (1 - IoU(other)) + positionCoef * posDist + sizeCoef * sizeDist +
    orientationDist * thetaCoef;
}

//------------------------------------------------------------------------------
double vtkLVBBoxDetection::GetVolume() const
{
  const auto size = this->GetSize();
  return size(0) * size(1) * size(2);
}

//------------------------------------------------------------------------------
double CalculatePolyArea(const std::vector<Vec2>& poly)
{
  double area = 0.0;
  for (size_t i = 0; i < poly.size(); ++i)
  {
    size_t j = (i + 1) % poly.size();
    area += poly[i].x() * poly[j].y() - poly[j].x() * poly[i].y();
  }

  return std::abs(area) * 0.5;
}

//------------------------------------------------------------------------------
// Sutherland-Hodgman algorithm: https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
std::vector<Vec2> ClipPolygon(const std::vector<Vec2>& poly, const Vec2& p1, const Vec2& p2)
{
  std::vector<Vec2> newPoly;
  Vec2 edge = p2 - p1;
  Vec2 normal(-edge.y(), edge.x());

  for (size_t i = 0; i < poly.size(); ++i)
  {
    const Vec2& a = poly[i];
    const Vec2& b = poly[(i + 1) % poly.size()];

    double distA = (a - p1).dot(normal);
    double distB = (b - p1).dot(normal);

    if (distA >= 0 && distB >= 0)
    {
      newPoly.push_back(b);
    }
    else if (distA >= 0 && distB < 0)
    {
      double t = distA / (distA - distB);
      newPoly.push_back(a + t * (b - a));
    }
    else if (distA < 0 && distB >= 0)
    {
      double t = distA / (distA - distB);
      newPoly.push_back(a + t * (b - a));
      newPoly.push_back(b);
    }
  }

  return newPoly;
}

std::vector<Vec2> vtkLVBBoxDetection::GetCorners2D() const
{
  std::vector<Vec2> corners;
  corners.reserve(4);
  double c = std::cos(this->Theta);
  double s = std::sin(this->Theta);
  Eigen::Matrix2d R;
  R << c, -s, s, c;

  Vec2 halfSize = this->Size.head<2>() * 0.5;
  Vec2 center = this->Position.head<2>();

  corners.push_back(center + R * Vec2(-halfSize.x(), -halfSize.y()));
  corners.push_back(center + R * Vec2(halfSize.x(), -halfSize.y()));
  corners.push_back(center + R * Vec2(halfSize.x(), halfSize.y()));
  corners.push_back(center + R * Vec2(-halfSize.x(), halfSize.y()));

  return corners;
}

//------------------------------------------------------------------------------
double vtkLVBBoxDetection::GetIntersection(const vtkLVBBoxDetection& other) const
{
  const Vec3& posA = this->Position;
  const Vec3& posB = other.Position;
  const Vec3& sizeA = this->Size;
  const Vec3& sizeB = other.Size;

  // We create an englobbing sphere, to avoid unnecessary computations
  double distSq = (posA - posB).squaredNorm();
  double maxDiagA = sizeA.norm() * 0.5;
  double maxDiagB = sizeB.norm() * 0.5;
  double minDist = maxDiagA + maxDiagB;
  if (distSq > minDist * minDist)
  {
    return 0.0;
  }

  // As our bboxes are only oriented along z-axis, we can check z-overlap
  double zminA = posA.z() - sizeA.z() * 0.5;
  double zmaxA = posA.z() + sizeA.z() * 0.5;
  double zminB = posB.z() - sizeB.z() * 0.5;
  double zmaxB = posB.z() + sizeB.z() * 0.5;
  double interZ = std::max(0.0, std::min(zmaxA, zmaxB) - std::max(zminA, zminB));

  if (interZ <= 0.0)
  {
    return 0.0;
  }

  std::vector<Vec2> polyA = this->GetCorners2D();
  std::vector<Vec2> polyB = other.GetCorners2D();

  std::vector<Vec2> result = polyA;
  for (int i = 0; i < 4; ++i)
  {
    result = ClipPolygon(result, polyB[i], polyB[(i + 1) % 4]);
    if (result.empty())
    {
      break;
    }
  }

  double area2D = CalculatePolyArea(result);
  double volume = area2D * interZ;

  return volume;
}

//------------------------------------------------------------------------------
double vtkLVBBoxDetection::IoU(const vtkLVBBoxDetection& other) const
{
  const double volumeA = this->GetVolume();
  const double volumeB = other.GetVolume();
  const double volumeIntersection = this->GetIntersection(other);
  const double volumeUnion = volumeA + volumeB - volumeIntersection;

  return volumeIntersection / volumeUnion;
}

//------------------------------------------------------------------------------
double vtkLVBBoxDetection::IoS(const vtkLVBBoxDetection& other) const
{
  auto otherScaled = vtkLVBBoxDetection(-1, other.Position, other.Size * 1.2, other.Theta);

  const double volumeIntersection = this->GetIntersection(otherScaled);
  const double volume = this->GetVolume();

  return volumeIntersection / volume;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkLVBBoxDetection::GetBoundingBoxPolyData() const
{
  vtkNew<vtkCubeSource> cubeSource;
  vtkNew<vtkPolyData> source;
  const auto center = this->Position;
  const auto size = this->Size;

  cubeSource->SetBounds(-size.x() / 2.0,
    size.x() / 2.0,
    -size.y() / 2.0,
    size.y() / 2.0,
    -size.z() / 2.0,
    size.z() / 2.0);
  cubeSource->Update();

  vtkNew<vtkTransform> transform;
  transform->PostMultiply();
  transform->RotateZ(Theta * 180.0 / vtkMath::Pi());
  transform->Translate(center.x(), center.y(), center.z());

  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetInputConnection(cubeSource->GetOutputPort());
  transformFilter->SetTransform(transform);
  transformFilter->Update();

  // Remove duplicated points in the cube source
  vtkNew<vtkCleanPolyData> clean;
  clean->SetInputConnection(transformFilter->GetOutputPort());
  clean->SetTolerance(0.000001);
  clean->Update();

  auto output = clean->GetOutput();
  output->SetFieldData(FieldData);

  return output;
}