/*=========================================================================

  Program: LidarView
  Module:  vtkAdaptiveOutlierRemoval.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// LOCAL
#include "vtkAdaptiveOutlierRemoval.h"
#include "KDTreeVTKAdaptor.h"

// VTK
#include <vtkCleanPolyData.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRemovePolyData.h>
#include <vtkTransform.h>

constexpr unsigned int POINTS_INPUT_PORT = 0;
constexpr unsigned int INPUT_PORT_COUNT = 1;

constexpr unsigned int FILTERED_POINTS_OUTPUT_PORT = 0;
constexpr unsigned int OUTPUT_PORT_COUNT = 1;

vtkStandardNewMacro(vtkAdaptiveOutlierRemoval)

//-----------------------------------------------------------------------------
vtkAdaptiveOutlierRemoval::vtkAdaptiveOutlierRemoval()
{
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);
  this->SetNumberOfOutputPorts(OUTPUT_PORT_COUNT);
}

//-----------------------------------------------------------------------------
int vtkAdaptiveOutlierRemoval::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkAdaptiveOutlierRemoval::RemoveOutlier(vtkSmartPointer<vtkPolyData> inputPointcloud,
  vtkSmartPointer<vtkPolyData> outputPointcloud)
{
  // Get depth array name
  this->DepthArrayName = this->GetInputArrayToProcess(0, inputPointcloud)
    ? this->GetInputArrayToProcess(0, inputPointcloud)->GetName()
    : "";

  // Get scalar name to use for outlier removal
  this->Scalar1ArrayName = this->GetInputArrayToProcess(1, inputPointcloud)
    ? this->GetInputArrayToProcess(1, inputPointcloud)->GetName()
    : "";
  this->Scalar2ArrayName = this->GetInputArrayToProcess(2, inputPointcloud)
    ? this->GetInputArrayToProcess(2, inputPointcloud)->GetName()
    : "";

  vtkKDTreeVTKAdaptor kDTree;
  std::vector<std::string> extraDims;
  std::vector<float> dimWeights(3, 1.0f);
  if (!this->Scalar1ArrayName.empty())
  {
    extraDims.emplace_back(this->Scalar1ArrayName);
    dimWeights.emplace_back(this->Scalar1Weight);
  }
  if (!this->Scalar2ArrayName.empty())
  {
    extraDims.emplace_back(this->Scalar2ArrayName);
    dimWeights.emplace_back(this->Scalar2Weight);
  }
  int dim = 3 + extraDims.size();
  kDTree.Reset(inputPointcloud, extraDims, dimWeights);
  vtkSmartPointer<vtkIdTypeArray> pointIdsToRemove = vtkSmartPointer<vtkIdTypeArray>::New();
  pointIdsToRemove->SetNumberOfComponents(1);
  for (auto id = 0; id < inputPointcloud->GetNumberOfPoints(); ++id)
  {
    double point[3];
    inputPointcloud->GetPoint(id, point);
    std::vector<float> ptXd(dim);
    ptXd[0] = point[0];
    ptXd[1] = point[1];
    ptXd[2] = point[2];
    int dimId = 3;
    for (const auto& arrayName : extraDims)
    {
      ptXd[dimId] = dimWeights[dimId] *
        inputPointcloud->GetPointData()->GetArray(arrayName.c_str())->GetTuple1(id);
      dimId++;
    }
    std::vector<float> sqDistances(this->NbNeighbors);
    std::vector<int> neighborsIndices(this->NbNeighbors);
    kDTree.KnnSearch(ptXd.data(), this->NbNeighbors, neighborsIndices, sqDistances);
    // Compute average distance
    float aveDist = 0.;
    for (const float& sqDist : sqDistances)
      aveDist += std::sqrt(sqDist);
    aveDist /= this->NbNeighbors;
    // Define the threshold
    double threshold = this->AveDistThreshold;
    if (this->EnableAdaptiveRemoval)
    {
      double depth = this->DepthArrayName.empty()
        ? std::sqrt(point[0] * point[0] + point[1] * point[1] + point[2] * point[2])
        : inputPointcloud->GetPointData()->GetArray(this->DepthArrayName.c_str())->GetTuple1(id);
      threshold = std::max(this->Factor * depth, this->AveDistThreshold);
    }
    // Save id of points to be removed
    if (aveDist > threshold)
      pointIdsToRemove->InsertNextValue(id);
  }
  vtkSmartPointer<vtkRemovePolyData> removePolyDataFilter =
    vtkSmartPointer<vtkRemovePolyData>::New();
  removePolyDataFilter->SetInputData(0, inputPointcloud);
  removePolyDataFilter->SetPointIds(pointIdsToRemove);

  // Clean removed points
  vtkSmartPointer<vtkCleanPolyData> cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanFilter->SetInputConnection(removePolyDataFilter->GetOutputPort());
  cleanFilter->PointMergingOff();
  cleanFilter->Update();

  outputPointcloud->ShallowCopy(cleanFilter->GetOutput());
}

//-----------------------------------------------------------------------------
int vtkAdaptiveOutlierRemoval::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get IO
  vtkPolyData* inputPointcloud = vtkPolyData::GetData(inputVector[POINTS_INPUT_PORT], 0);
  vtkPolyData* outputPointcloud = vtkPolyData::GetData(outputVector, FILTERED_POINTS_OUTPUT_PORT);

  this->RemoveOutlier(inputPointcloud, outputPointcloud);
  return 1;
}
