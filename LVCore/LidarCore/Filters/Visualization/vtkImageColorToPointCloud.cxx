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

  auto idx = [&](int i, int j, int k) -> vtkIdType
  { return static_cast<vtkIdType>(i + dims[0] * (j + dims[1] * k)); };

  // Basis vectors for the grid (if available)
  double p00[3] = { 0.0, 0.0, 0.0 };
  double dx[3] = { 0.0, 0.0, 0.0 };
  double dy[3] = { 0.0, 0.0, 0.0 };

  // Precompute 2x2 inverse metric for projection to (i,j)
  double a00 = 0.0, a11 = 0.0;
  double inv00 = 0.0, inv11 = 0.0;
  if (gridPoints && dims[0] >= 1 && dims[1] >= 1)
  {
    gridPoints->GetPoint(idx(0, 0, 0), p00);
    if (dims[0] >= 2)
    {
      double p10[3];
      gridPoints->GetPoint(idx(1, 0, 0), p10);
      // World-space displacement of one grid step along the X (i) axis
      dx[0] = p10[0] - p00[0];
      dx[1] = p10[1] - p00[1];
      dx[2] = p10[2] - p00[2];
    }
    if (dims[1] >= 2)
    {
      double p01[3];
      gridPoints->GetPoint(idx(0, 1, 0), p01);
      // World-space displacement of one grid step along the Y (j) axis
      dy[0] = p01[0] - p00[0];
      dy[1] = p01[1] - p00[1];
      dy[2] = p01[2] - p00[2];
    }
  }

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

  // Compute the world-space distance of a one-pixel step along the X/Y directions
  a00 = dx[0] * dx[0] + dx[1] * dx[1] + dx[2] * dx[2];
  a11 = dy[0] * dy[0] + dy[1] * dy[1] + dy[2] * dy[2];

  // Convert a unit world-space displacement to pixel displacement along X and Y
  inv00 = 1.0 / a00;
  inv11 = 1.0 / a11;

  vtkDataArray* colorArray = nullptr;
  if (auto rgbUChar = vtkUnsignedCharArray::SafeDownCast(sgrid->GetPointData()->GetArray("RGB")))
  {
    colorArray = rgbUChar;
  }
  else
  {
    colorArray = sgrid->GetPointData()->GetScalars();
  }

  unsigned char* outColors = static_cast<unsigned char*>(colors->GetVoidPointer(0));
  const vtkIdType npts = points->GetNumberOfPoints();
  vtkSMPTools::For(0,
    npts,
    [&](vtkIdType begin, vtkIdType end)
    {
      double p[3];
      for (vtkIdType pid = begin; pid < end; ++pid)
      {
        points->GetPoint(pid, p);
        vtkIdType gid = -1;
        // Project vector v = p - p00 onto the grid basis to get (i,j)
        double v[3] = { p[0] - p00[0], p[1] - p00[1], p[2] - p00[2] };

        // Distance of the point displacement projected onto the image X/Y axes (in world units)
        const double b0 = dx[0] * v[0] + dx[1] * v[1] + dx[2] * v[2];
        const double b1 = dy[0] * v[0] + dy[1] * v[1] + dy[2] * v[2];

        // Convert world-space distances along image X/Y directions to pixel offsets
        const double ci = inv00 * b0;
        const double cj = inv11 * b1;

        // Nearest-neighbor sampling in index space
        long ii = static_cast<long>(std::lround(ci));
        long jj = static_cast<long>(std::lround(cj));

        if (ii >= 0 && jj >= 0 && ii < dims[0] && jj < dims[1])
        {
          gid = idx(static_cast<int>(ii), static_cast<int>(jj), 0);
        }
        unsigned char rgb[3] = { 255, 255, 255 };
        if (gid >= 0 && colorArray)
        {
          if (auto uc = vtkUnsignedCharArray::SafeDownCast(colorArray))
          {
            unsigned char tmp[4] = { 0, 0, 0, 0 };
            uc->GetTypedTuple(gid, tmp);
            rgb[0] = tmp[0];
            rgb[1] = uc->GetNumberOfComponents() > 1 ? tmp[1] : tmp[0];
            rgb[2] = uc->GetNumberOfComponents() > 2 ? tmp[2] : tmp[0];
          }
          else if (auto da = vtkDataArray::SafeDownCast(colorArray))
          {
            double colorR = da->GetComponent(gid, 0);
            double colorG = da->GetNumberOfComponents() > 1 ? da->GetComponent(gid, 1) : colorR;
            double colorB = da->GetNumberOfComponents() > 2 ? da->GetComponent(gid, 2) : colorR;
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
