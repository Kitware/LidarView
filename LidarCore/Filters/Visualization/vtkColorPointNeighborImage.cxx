/*=========================================================================

  Program:   LidarView
  Module:    vtkColorPointNeighborImage.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkColorPointNeighborImage.h"

#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointLocator.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSMPTools.h>
#include <vtkStaticPointLocator.h>
#include <vtkUnsignedCharArray.h>

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkColorPointNeighborImage);
namespace
{
constexpr int POINTS_INPUT_PORT = 0;
constexpr int IMAGE_INPUT_PORT = 1;
}

//-----------------------------------------------------------------------------
vtkColorPointNeighborImage::vtkColorPointNeighborImage()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkColorPointNeighborImage::~vtkColorPointNeighborImage() = default;

//-----------------------------------------------------------------------------
int vtkColorPointNeighborImage::FillInputPortInformation(int port, vtkInformation* info)
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
int vtkColorPointNeighborImage::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* imgInfo = inputVector[IMAGE_INPUT_PORT]->GetInformationObject(0);
  vtkDataSet* sgrid = vtkDataSet::GetData(imgInfo);
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
  const char* outName = this->GetOutputArrayName();
  if (!outName)
  {
    outName = "RGB";
  }
  colors->SetName(outName);
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(points->GetNumberOfPoints());

  // Build a locator for nearest neighbor color transfer
  vtkNew<vtkStaticPointLocator> locator;
  locator->SetDataSet(sgrid);
  locator->BuildLocator();

  vtkDataArray* colorArray = nullptr;
  const char* colorName = this->GetColorArrayName();
  if (colorName && *colorName)
  {
    colorArray = sgrid->GetPointData()->GetArray(colorName);
  }

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
      double p[3];
      for (vtkIdType pid = begin; pid < end; ++pid)
      {
        points->GetPoint(pid, p);
        vtkIdType gid = locator->FindClosestPoint(p);
        unsigned char rgb[3] = { 255, 255, 255 };
        if (gid >= 0 && colorArray)
        {
          if (auto ucharArray = vtkUnsignedCharArray::SafeDownCast(colorArray))
          {
            unsigned char tmp[4] = { 0, 0, 0, 0 };
            ucharArray->GetTypedTuple(gid, tmp);
            if (this->ImageType == Mask)
            {
              // Convert a mask to a binary grayscale RGB pixel
              const bool isForeground = (tmp[0] > 0) ||
                (ucharArray->GetNumberOfComponents() > 1 && tmp[1] > 0) ||
                (ucharArray->GetNumberOfComponents() > 2 && tmp[2] > 0);
              const unsigned char maskValue = isForeground ? 255 : 0;
              rgb[0] = maskValue;
              rgb[1] = maskValue;
              rgb[2] = maskValue;
            }
            else if (this->ImageType == Grayscale)
            {
              rgb[0] = tmp[0];
              rgb[1] = tmp[0];
              rgb[2] = tmp[0];
            }
            else // ColorImage
            {
              rgb[0] = tmp[0];
              rgb[1] = tmp[1];
              rgb[2] = tmp[2];
            }
          }
          else if (auto dataArray = vtkDataArray::SafeDownCast(colorArray))
          {
            if (this->ImageType == Mask)
            {
              // Threshold the scalar mask (v > 0.5) to generate a binary grayscale RGB pixel.
              const double v = dataArray->GetComponent(gid, 0);
              const unsigned char maskValue = (v > 0.5) ? 255 : 0;
              rgb[0] = maskValue;
              rgb[1] = maskValue;
              rgb[2] = maskValue;
            }
            else if (this->ImageType == Grayscale)
            {
              const double v = dataArray->GetComponent(gid, 0);
              const unsigned char grayValue =
                static_cast<unsigned char>(std::lround(std::max(0.0, std::min(255.0, v))));
              rgb[0] = grayValue;
              rgb[1] = grayValue;
              rgb[2] = grayValue;
            }
            else // ColorImage
            {
              double r = dataArray->GetComponent(gid, 0);
              double g = dataArray->GetComponent(gid, 1);
              double b = dataArray->GetComponent(gid, 2);
              rgb[0] = static_cast<unsigned char>(std::lround(std::max(0.0, std::min(255.0, r))));
              rgb[1] = static_cast<unsigned char>(std::lround(std::max(0.0, std::min(255.0, g))));
              rgb[2] = static_cast<unsigned char>(std::lround(std::max(0.0, std::min(255.0, b))));
            }
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
