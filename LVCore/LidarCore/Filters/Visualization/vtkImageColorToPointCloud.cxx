/*=========================================================================

  Program:   LidarView
  Module:    vtkImageColorToPointCloud.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageColorToPointCloud.h"

#include <vtkDataArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSMPTools.h>
#include <vtkStructuredGrid.h>
#include <vtkUnsignedCharArray.h>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int POINTS_INPUT_PORT = 0;
constexpr int IMAGE_INPUT_PORT = 1;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageColorToPointCloud);

//-----------------------------------------------------------------------------
vtkImageColorToPointCloud::vtkImageColorToPointCloud()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkImageColorToPointCloud::~vtkImageColorToPointCloud() = default;

//-----------------------------------------------------------------------------
int vtkImageColorToPointCloud::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == POINTS_INPUT_PORT)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }

  if (port == IMAGE_INPUT_PORT)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
  }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkImageColorToPointCloud::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* imgInfo = inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0);
  vtkStructuredGrid* sgrid = vtkStructuredGrid::GetData(imgInfo);
  vtkPolyData* inputPoly =
    vtkPolyData::GetData(inputVector[POINTS_INPUT_PORT]->GetInformationObject(0));
  vtkPolyData* outputPoly = vtkPolyData::GetData(outputVector->GetInformationObject(0));

  if (!sgrid || !inputPoly)
  {
    vtkErrorMacro("Missing input: need vtkPolyData at port 0 and vtkDataSet at port 1");
    return 0;
  }

  outputPoly->ShallowCopy(inputPoly);

  vtkPoints* points = inputPoly->GetPoints();
  if (!points || points->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  // Prepare output color array
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetName("RGB");
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(points->GetNumberOfPoints());

  // Infer a regular 2D grid basis from the structured grid
  int dims[3] = { 0, 0, 0 };
  sgrid->GetDimensions(dims);
  vtkPoints* gridPoints = sgrid->GetPoints();

  // Basis points
  double gridOrigin[3] = { 0.0, 0.0, 0.0 };
  double gridPointAlongI[3] = { 0.0, 0.0, 0.0 };
  double gridPointAlongJ[3] = { 0.0, 0.0, 0.0 };

  // Validate grid is 2D and regular in i,j
  if (dims[2] != 1)
  {
    vtkErrorMacro("Structured grid must be 2D (dims[2] == 1).");
    return 0;
  }
  if (!(gridPoints && dims[0] >= 2 && dims[1] >= 2))
  {
    vtkErrorMacro("Structured grid must be at least 2x2 in i and j.");
    return 0;
  }

  auto idx = [&](int i, int j, int k) -> vtkIdType
  { return static_cast<vtkIdType>(i + dims[0] * (j + dims[1] * k)); };

  // Retrieve three reference points from the structured grid to define the 2D grid basis:
  // gridOrigin: origin of the grid (i=0, j=0)
  // gridPointAlongI: one-step increment along the i direction
  // gridPointAlongJ: one-step increment along the j direction
  // These points are later used to infer the grid spacing and orientation in world coordinates.
  sgrid->GetPoint(idx(0, 0, 0), gridOrigin);
  sgrid->GetPoint(idx(1, 0, 0), gridPointAlongI);
  sgrid->GetPoint(idx(0, 1, 0), gridPointAlongJ);

  // Select the two world axes defining the projection plane by excluding DropAxis.
  // Mapping:
  //   DropAxis = 0 (X) -> planeAxes = {1, 2}  // YZ plane
  //   DropAxis = 1 (Y) -> planeAxes = {0, 2}  // XZ plane
  //   DropAxis = 2 (Z) -> planeAxes = {0, 1}  // XY plane
  int planeAxes[2] = { 0, 1 };
  int planeCandidates[2];
  int candidateCount = 0;
  for (int axis = 0; axis < 3; ++axis)
  {
    if (axis != this->DropAxis)
    {
      planeCandidates[candidateCount++] = axis;
    }
  }
  planeAxes[0] = planeCandidates[0];
  planeAxes[1] = planeCandidates[1];

  // Project the 3D grid basis vectors (gridPointAlongI-gridOrigin, gridPointAlongJ-gridOrigin)
  // onto the
  // 2D plane defined by planeAxes, yielding a 2x2 basis for (i,j).
  const double gridBasisIPlaneX = gridPointAlongI[planeAxes[0]] - gridOrigin[planeAxes[0]];
  const double gridBasisIPlaneY = gridPointAlongI[planeAxes[1]] - gridOrigin[planeAxes[1]];
  const double gridBasisJPlaneX = gridPointAlongJ[planeAxes[0]] - gridOrigin[planeAxes[0]];
  const double gridBasisJPlaneY = gridPointAlongJ[planeAxes[1]] - gridOrigin[planeAxes[1]];

  // Determinant of the 2D basis matrix [dx dy] on the projection plane.
  const double basisDeterminant =
    gridBasisIPlaneX * gridBasisJPlaneY - gridBasisIPlaneY * gridBasisJPlaneX;

  const double inverseBasisDeterminant = 1.0 / basisDeterminant;

  const std::string requestedName =
    this->ColorArrayName.empty() ? std::string("PNGImage") : this->ColorArrayName;
  vtkDataArray* colorArray = sgrid->GetPointData()->GetArray(requestedName.c_str());
  if (!colorArray)
  {
    colorArray = sgrid->GetPointData()->GetScalars();
  }

  unsigned char* outColors = static_cast<unsigned char*>(colors->GetVoidPointer(0));
  const vtkIdType npts = points->GetNumberOfPoints();
  vtkSMPTools::For(0,
    npts,
    [&](vtkIdType begin, vtkIdType end)
    {
      double point[3];
      for (vtkIdType pid = begin; pid < end; ++pid)
      {
        points->GetPoint(pid, point);
        vtkIdType imagePointId = -1;

        // pointOffset = point - gridOrigin: displacement of the 3D point from the grid origin,
        // expressed in the 2D projection plane, used to solve for (i, j).
        const double pointOffsetPlaneX = point[planeAxes[0]] - gridOrigin[planeAxes[0]];
        const double pointOffsetPlaneY = point[planeAxes[1]] - gridOrigin[planeAxes[1]];

        double continuousI = 0.0, continuousJ = 0.0;

        // Solve the 2x2 linear system [dx dy] * [i j]^T = pointOffset
        // to recover the continuous pixel coordinates (i, j)
        // in the projected 2D grid.
        continuousI =
          (pointOffsetPlaneX * gridBasisJPlaneY - pointOffsetPlaneY * gridBasisJPlaneX) *
          inverseBasisDeterminant;
        continuousJ =
          (-pointOffsetPlaneX * gridBasisIPlaneY + pointOffsetPlaneY * gridBasisIPlaneX) *
          inverseBasisDeterminant;

        // Nearest-neighbor sampling in index space
        long nearestI = static_cast<long>(std::lround(continuousI));
        long nearestJ = static_cast<long>(std::lround(continuousJ));

        if (nearestI >= 0 && nearestJ >= 0 && nearestI < dims[0] && nearestJ < dims[1])
        {
          imagePointId = idx(static_cast<int>(nearestI), static_cast<int>(nearestJ), 0);
        }
        unsigned char rgb[3] = { 255, 255, 255 };
        if (imagePointId >= 0 && colorArray)
        {
          if (auto unsignedCharColorArray = vtkUnsignedCharArray::SafeDownCast(colorArray))
          {
            unsigned char colorTuple[4] = { 0, 0, 0, 0 };
            unsignedCharColorArray->GetTypedTuple(imagePointId, colorTuple);
            rgb[0] = colorTuple[0];
            rgb[1] =
              unsignedCharColorArray->GetNumberOfComponents() > 1 ? colorTuple[1] : colorTuple[0];
            rgb[2] =
              unsignedCharColorArray->GetNumberOfComponents() > 2 ? colorTuple[2] : colorTuple[0];
          }
          else if (auto numericColorArray = vtkDataArray::SafeDownCast(colorArray))
          {
            double colorR = numericColorArray->GetComponent(imagePointId, 0);
            double colorG = numericColorArray->GetNumberOfComponents() > 1
              ? numericColorArray->GetComponent(imagePointId, 1)
              : colorR;
            double colorB = numericColorArray->GetNumberOfComponents() > 2
              ? numericColorArray->GetComponent(imagePointId, 2)
              : colorR;
            rgb[0] =
              static_cast<unsigned char>(std::lround(std::max(0.0, std::min(255.0, colorR))));
            rgb[1] =
              static_cast<unsigned char>(std::lround(std::max(0.0, std::min(255.0, colorG))));
            rgb[2] =
              static_cast<unsigned char>(std::lround(std::max(0.0, std::min(255.0, colorB))));
          }
        }
        const vtkIdType base = 3 * pid;
        outColors[base + 0] = rgb[0];
        outColors[base + 1] = rgb[1];
        outColors[base + 2] = rgb[2];
      }
    });

  outputPoly->GetPointData()->SetScalars(colors);

  return 1;
}
