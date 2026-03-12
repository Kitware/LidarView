/*=========================================================================

  Program: LidarView
  Module:  vtkComputeVolume.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// LOCAL
#include "vtkComputeVolume.h"
#include "vtkConversions.h"

// VTK
#include <vtkCenterOfMass.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkInformation.h>
#include <vtkMath.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTableAlgorithm.h>
#include <vtkTransform.h>

// EIGEN
#include <Eigen/Dense>

// STD
#include <climits>
#include <string>

class vtkComputeVolume::vtkInternals
{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  /**
   * Size of the leaf used to rasterize the pointcloud (in meters)
   * It should be equivalent to or greater than the average resolution of the input pointcloud
   */
  double GridResolution = 0.001;

  /**
   * Map containing one raster bin index and the point in this bin
   */
  std::unordered_map<int, Eigen::Vector3d> Raster;

  /**
   * Size of the raster grid { X-direction, Y-direction }
   * This parameter is automatically computed from the bounds and the leaf
   * Size: NbinsX * NbinsY
   */
  Eigen::Vector2i GridSize = { 100, 100 };

  /**
   * The inputs data of a plane are the 4 vertices of the rectangle and the plane normal
   * An isometry of the plane reference, the length and the width are computed by inputs
   */
  class Plane
  {
  public:
    // Getters / Setters of the plane
    void SetPlane(const std::vector<Eigen::Vector3d>& planePts, const Eigen::Vector3d& normal)
    {
      const Eigen::Vector3d& origin = planePts[0];
      // Compute vectors from origin
      double minDist = DBL_MAX;
      Eigen::Vector3d axisX, axisY = { 0., 0., 0. };
      for (int i = 1; i < 4; ++i)
      {
        Eigen::Vector3d vec = planePts[i] - origin;
        double dist = vec.norm();
        // Include the square case
        if (dist <= minDist)
        {
          axisX = axisY;
          axisY = vec;
          minDist = dist;
        }
      }
      this->Length = axisX.norm();
      this->Width = axisY.norm();
      this->Reference.translation() = origin;
      this->Reference.linear().col(0) = axisX.normalized();
      this->Reference.linear().col(1) = axisY.normalized();
      this->Reference.linear().col(2) = normal.normalized();
    }

    Eigen::Isometry3d GetReference() { return this->Reference; }
    double GetLength() { return this->Length; }
    double GetWidth() { return this->Width; }

  private:
    Eigen::Isometry3d Reference = Eigen::Isometry3d::Identity();
    // On X axis
    double Length;
    // On Y axis
    double Width;
  };
  Plane InputPlane;

  //----------------------------------------------------------------------------
  int ToOneDimension(int xId, int yId)
  {
    // Fill Y direction first, GridSize = {xSize, ySize}
    return xId * this->GridSize[1] + yId;
  }

  //----------------------------------------------------------------------------
  Eigen::Vector2i ToTwoDimensions(int index)
  {
    // Fill Y direction first, GridSize = {xSize, ySize}
    return { index / this->GridSize[1], index % this->GridSize[1] };
  }

  //----------------------------------------------------------------------------
  Eigen::Vector2i ProjectInGrid(const Eigen::Vector3d& pt)
  {
    // return {xId, yId}
    return (pt / this->GridResolution).cast<int>().head(2);
  }

  //----------------------------------------------------------------------------
  void RasterizePointcloud(const std::vector<Eigen::Vector3d>& pointcloud)
  {
    this->Raster.clear();
    // Compute grid size
    this->GridSize = { this->InputPlane.GetLength() / this->GridResolution,
      this->InputPlane.GetWidth() / this->GridResolution };

    // Helper to check if a point lays in the grid
    auto isPointInSpace = [this](const Eigen::Vector3d& point)
    {
      // clang-format off
      return point.z() >  0 &&
             point.x() >= 0 && point.x() <= (this->GridSize[0] * this->GridResolution) &&
             point.y() >= 0 && point.y() <= (this->GridSize[1] * this->GridResolution);
      // clang-format on
    };
    // Fill raster
    for (const auto& pt : pointcloud)
    {
      Eigen::Vector2i coord2D = this->ProjectInGrid(pt);
      int id1D = this->ToOneDimension(coord2D(0), coord2D(1));
      if (isPointInSpace(pt) && (this->Raster.count(id1D) == 0 || pt.z() > this->Raster[id1D].z()))
      {
        this->Raster[id1D] = pt;
      }
    }
  }

  //----------------------------------------------------------------------------
  std::pair<int, int> FindBoundPoint(int col, int row, bool searchInRow)
  {
    // Given a bin at position (row, col), find the first bins which are not empty
    // at the left and at the right of the scan line
    // If the row = 0 when searchInRow is true, this function returns the first and the last
    // non-empty points at this column. If the col = 0 when searchInRow is false, this function
    // returns the first and the last non-empty points at this row
    int MaxSize = searchInRow ? this->GridSize[1] : this->GridSize[0];
    int rightBound = searchInRow ? row + 1 : col + 1;
    int rightBound1D =
      searchInRow ? this->ToOneDimension(col, rightBound) : this->ToOneDimension(rightBound, row);
    while (this->Raster.count(rightBound1D) == 0 && rightBound < (MaxSize - 1))
    {
      rightBound++;
      rightBound1D =
        searchInRow ? this->ToOneDimension(col, rightBound) : this->ToOneDimension(rightBound, row);
    }

    int leftBound = searchInRow ? row - 1 : col - 1;
    leftBound = leftBound < 0 ? (MaxSize - 1) : leftBound;
    int leftBound1D =
      searchInRow ? this->ToOneDimension(col, leftBound) : this->ToOneDimension(leftBound, row);
    while (this->Raster.count(leftBound1D) == 0 && leftBound > 0)
    {
      leftBound--;
      leftBound1D =
        searchInRow ? this->ToOneDimension(col, leftBound) : this->ToOneDimension(leftBound, row);
    }
    return std::make_pair(std::min(leftBound, rightBound), std::max(leftBound, rightBound));
  }

  //----------------------------------------------------------------------------
  void Interpolate1DGrid(int interpoThreshold)
  {
    // Interpolation for each column
    for (int col = 0; col < this->GridSize[0]; col++)
    {
      // Find the first and the last points for the column
      auto bound = this->FindBoundPoint(col, 0, true);
      // Interpolate the empty area in the column
      for (int row = bound.first; row <= bound.second; row++)
      {
        int id1D = this->ToOneDimension(col, row);
        if (this->Raster.count(id1D) == 0)
        {
          auto interpoBound = this->FindBoundPoint(col, row, true);
          int interval = interpoBound.second - interpoBound.first;
          if (interval < interpoThreshold)
          {
            Eigen::Vector3d leftPoint =
              this->Raster.at(this->ToOneDimension(col, interpoBound.first));
            Eigen::Vector3d interpolationVector =
              this->Raster.at(this->ToOneDimension(col, interpoBound.second)) - leftPoint;
            for (int i = 1; i < interval; i++)
            {
              this->Raster[this->ToOneDimension(col, interpoBound.first + i)] =
                leftPoint + (double(i) / double(interval) * interpolationVector);
            }
          }
          // Update row id for the next check
          row = interpoBound.second;
        }
      }
    }

    // Interpolation for each row
    for (int row = 0; row < this->GridSize[1]; row++)
    {
      // Find the first and the last points for the row
      auto bound = this->FindBoundPoint(0, row, false);
      // Interpolate the empty area in the row
      for (int col = bound.first; col <= bound.second; col++)
      {
        int id1D = this->ToOneDimension(col, row);
        if (this->Raster.count(id1D) == 0)
        {
          auto interpoBound = this->FindBoundPoint(col, row, false);
          int interval = interpoBound.second - interpoBound.first;
          if (interval < interpoThreshold)
          {
            Eigen::Vector3d leftPoint =
              this->Raster.at(this->ToOneDimension(interpoBound.first, row));
            Eigen::Vector3d interpolationVector =
              this->Raster.at(this->ToOneDimension(interpoBound.second, row)) - leftPoint;
            for (int i = 1; i < interval; i++)
            {
              this->Raster[this->ToOneDimension(interpoBound.first + i, row)] =
                leftPoint + (double(i) / double(interval) * interpolationVector);
            }
          }
          // Update column id for the next check
          col = interpoBound.second;
        }
      }
    }
  }

  //----------------------------------------------------------------------------
  void Interpolate2DGrid(int interpoRadius)
  {
    // Interpolation for each column
    std::vector<int> occupancyMap(this->GridSize[0] * this->GridSize[1], -1);

    for (const auto& binPt : this->Raster)
    {
      int id1D = binPt.first;
      occupancyMap[id1D] = 1;
      Eigen::Vector2i id2D = this->ToTwoDimensions(id1D);
      // Do dilatation to define the pixels to interpolate
      for (int col = id2D[0] - interpoRadius; col <= id2D[0] + interpoRadius; col++)
      {
        for (int row = id2D[1] - interpoRadius; row <= id2D[1] + interpoRadius; row++)
        {
          if (col < 0 || col >= this->GridSize[0] || row < 0 || row >= this->GridSize[1] ||
            (col == id2D[0] && row == id2D[1]))
            continue;
          int checkId = this->ToOneDimension(col, row);
          if (this->Raster.count(checkId) == 0)
          {
            occupancyMap[checkId] = 0;
          }
        }
      }
    }
    // Compute value for points to interpolate
    for (std::vector<int>::size_type id = 0; id < occupancyMap.size(); id++)
    {
      if (occupancyMap[id] != 0)
        continue;
      Eigen::Vector3d interpoPt = { 0., 0., 0. };
      double sumWeight = 0;
      Eigen::Vector2i id2D = this->ToTwoDimensions(id);
      for (int col = id2D[0] - interpoRadius; col <= id2D[0] + interpoRadius; col++)
      {
        for (int row = id2D[1] - interpoRadius; row <= id2D[1] + interpoRadius; row++)
        {
          if (col < 0 || col >= this->GridSize[0] || row < 0 || row >= this->GridSize[1] ||
            (col == id2D[0] && row == id2D[1]))
            continue;
          int checkId = this->ToOneDimension(col, row);
          if (occupancyMap[checkId] == 1)
          {
            double weight = exp(-0.5 *
              (std::pow(double(col - id2D[0]), 2.0) + std::pow(double(row - id2D[1]), 2.0)) /
              std::pow(double(interpoRadius / 2.5), 2.0));
            interpoPt += weight * this->Raster.at(checkId);
            sumWeight += weight;
          }
        }
      }
      if (sumWeight > 0)
      {
        interpoPt /= sumWeight;
        this->Raster[id] = interpoPt;
      }
    }
  }

  //----------------------------------------------------------------------------
  double ComputeIntegralVolume()
  {
    if (this->Raster.empty())
      return 0.;

    // Sum the height values
    double sumHeight = 0.;
    for (const auto& binPt : this->Raster)
      sumHeight += binPt.second.z();

    // Multiply height values by the bin surface area to get the volume
    double areaBin = std::pow(this->GridResolution, 2.0);
    return areaBin * sumHeight;
  }
};

constexpr unsigned int POINTS_INPUT_PORT = 0;
constexpr unsigned int PLANE_INPUT_PORT = 1;
constexpr unsigned int INPUT_PORT_COUNT = 2;

constexpr unsigned int RASTERIZED_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int VOLUME_OUTPUT_PORT = 1;
constexpr unsigned int OUTPUT_PORT_COUNT = 2;

vtkStandardNewMacro(vtkComputeVolume)

//-----------------------------------------------------------------------------
vtkComputeVolume::vtkComputeVolume()
  : Internals(new vtkComputeVolume::vtkInternals())
{
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//-----------------------------------------------------------------------------
int vtkComputeVolume::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkComputeVolume::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == RASTERIZED_POINTS_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  if (port == VOLUME_OUTPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkComputeVolume::SetGridResolution(double resolution)
{
  if (resolution < 0)
  {
    vtkErrorMacro("Invalid input resolution '" << resolution << "'.");
    return;
  }

  if (this->Internals->GridResolution != resolution)
  {
    this->Internals->GridResolution = resolution;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkComputeVolume::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Internals)
    return 0;

  // Step 1: Get IO
  vtkPolyData* inCloud = vtkPolyData::GetData(inputVector[POINTS_INPUT_PORT], 0);
  vtkPolyData* inPlane = vtkPolyData::GetData(inputVector[PLANE_INPUT_PORT], 0);
  vtkPolyData* rasterizedCloud = vtkPolyData::GetData(outputVector, RASTERIZED_POINTS_OUTPUT_PORT);
  vtkTable* volumeOutput = vtkTable::GetData(outputVector, VOLUME_OUTPUT_PORT);
  // Check if input plane is a multiblock
  if (!inPlane)
  {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::GetData(inputVector[PLANE_INPUT_PORT], 0);
    // Extract first block if it is a vtkPolyData
    inPlane = vtkPolyData::SafeDownCast(mb->GetBlock(0));
  }

  if (!inCloud || !inPlane)
  {
    vtkErrorMacro("No input data");
    return 0;
  }

  if (inPlane->GetNumberOfPoints() != 4 || !inPlane->GetPointData()->GetArray("Normals"))
  {
    vtkErrorMacro("No valid plane data");
    return 0;
  }

  // Get plane normal and 4 vertices
  Eigen::Vector3d planeNormal;
  inPlane->GetPointData()->GetArray("Normals")->GetTuple(0, planeNormal.data());
  std::vector<Eigen::Vector3d> planePts(4);
  for (auto ptIdx = 0; ptIdx < 4; ++ptIdx)
  {
    inPlane->GetPoint(ptIdx, planePts[ptIdx].data());
  }

  // Get pointcloud centroid
  Eigen::Vector3d pointcloudCentroid;
  vtkSmartPointer<vtkCenterOfMass> centerOfMassFilter = vtkSmartPointer<vtkCenterOfMass>::New();
  centerOfMassFilter->SetInputData(inCloud);
  centerOfMassFilter->SetUseScalarsAsWeights(false);
  centerOfMassFilter->Update();
  centerOfMassFilter->GetCenter(pointcloudCentroid.data());
  // Orient normal towards the cloud
  Eigen::Vector3d pointsDirection = pointcloudCentroid - planePts[0];
  if (planeNormal.dot(pointsDirection) < 0.)
    planeNormal *= -1.0;
  this->Internals->InputPlane.SetPlane(planePts, planeNormal);

  // Convert the input pointcloud in Eigen data structure pointcloud
  std::vector<Eigen::Vector3d> points = vtkPointsToEigenVector(inCloud->GetPoints());
  // Transform input pointcloud into the input plane reference
  Eigen::Isometry3d transform = this->Internals->InputPlane.GetReference().inverse();
  for (auto& pt : points)
    pt = transform * pt;

  // Step 2: Rasterize pointcloud
  this->Internals->RasterizePointcloud(points);
  // Interpolate empty bins to improve the estimation
  if (this->EnableInterpolation)
    this->Internals->Interpolate2DGrid(this->InterpolationThreshold);

  // Step 3: Compute integral volume
  double volume = this->Internals->ComputeIntegralVolume();

  // Set output pointclouds
  vtkIdType pointIndex = 0;
  vtkIdType nbBins = this->Internals->Raster.size();
  vtkNew<vtkPoints> rasterizedPoints;
  rasterizedPoints->SetNumberOfPoints(nbBins);
  rasterizedCloud->SetPoints(rasterizedPoints);

  // Init and register cells
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(nbBins);
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(1, connectivity);
  rasterizedCloud->SetVerts(cellArray);

  Eigen::Isometry3d transformInverse = this->Internals->InputPlane.GetReference();
  for (const auto& bin : this->Internals->Raster)
  {
    connectivity->SetValue(pointIndex, pointIndex);

    Eigen::Vector3d originPt = transformInverse * bin.second;
    rasterizedPoints->SetPoint(pointIndex, originPt.data());
    ++pointIndex;
  }

  // Add volume into field data of outputs
  vtkDoubleArray* volumeArray = vtkDoubleArray::New();
  volumeArray->SetName("Volume");
  volumeArray->SetNumberOfComponents(1);
  volumeArray->SetNumberOfTuples(1);
  volumeArray->InsertComponent(0, 0, volume);
  rasterizedCloud->GetFieldData()->AddArray(volumeArray);
  volumeArray->Delete();

  // Set output for displaying volume information
  std::ostringstream oss;
  oss << std::setprecision(6) << std::noshowpoint << volume << " m3";
  std::string volumText = "The volume is estimated to " + oss.str();
  vtkNew<vtkStringArray> data;
  data->SetName("Volume Information");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(volumText.c_str());
  volumeOutput->AddColumn(data);

  return 1;
}
