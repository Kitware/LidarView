/*=========================================================================

  Program: LidarView
  Module:  vtkCalibrationEvaluation.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// LOCAL
#include "vtkCalibrationEvaluation.h"
#include "vtkAggregatePointsFromTrajectoryOffline.h"

// VTK
#include <unordered_set>
#include <vector>
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

// Lidar IO for upstream calibration fetch
#include "vtkLidarPacketInterpreter.h"
#include "vtkLidarReader.h"
#include <string>
#include <vtkDoubleArray.h>
#include <vtkTable.h>

// Port indices
constexpr unsigned int LIDAR_INPUT_PORT = 0;
constexpr unsigned int TRAJ_INPUT_PORT = 1;
constexpr unsigned int INPUT_PORT_COUNT = 2;

// Output ports
constexpr unsigned int TABLE_OUTPUT_PORT = 0;
constexpr unsigned int AGGREGATED_CLOUD_OUTPUT_PORT = 1;
constexpr unsigned int OUTPUT_PORT_COUNT = 2;

vtkStandardNewMacro(vtkCalibrationEvaluation)

//-----------------------------------------------------------------------------
namespace
{
vtkSmartPointer<vtkTransform> FindUpstreamSensorTransform(vtkAlgorithmOutput* inConn)
{
  if (!inConn)
  {
    return nullptr;
  }

  std::unordered_set<vtkAlgorithm*> visited;
  std::vector<vtkAlgorithm*> stack;
  stack.push_back(inConn->GetProducer());

  while (!stack.empty())
  {
    vtkAlgorithm* alg = stack.back();
    stack.pop_back();
    if (!alg || visited.count(alg))
    {
      continue;
    }
    visited.insert(alg);

    if (auto reader = vtkLidarReader::SafeDownCast(alg))
    {
      auto interp = reader->GetLidarInterpreter();
      if (interp && interp->GetSensorTransform())
      {
        vtkSmartPointer<vtkTransform> t;
        t.TakeReference(vtkTransform::New());
        t->DeepCopy(interp->GetSensorTransform());
        return t;
      }
    }

    int nports = alg->GetNumberOfInputPorts();
    for (int p = 0; p < nports; ++p)
    {
      int nconn = alg->GetNumberOfInputConnections(p);
      for (int c = 0; c < nconn; ++c)
      {
        vtkAlgorithmOutput* up = alg->GetInputConnection(p, c);
        if (up && up->GetProducer())
        {
          stack.push_back(up->GetProducer());
        }
      }
    }
  }

  return nullptr;
}
}

//-----------------------------------------------------------------------------
vtkCalibrationEvaluation::vtkCalibrationEvaluation()
{
  this->SetNumberOfInputPorts(INPUT_PORT_COUNT);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
int vtkCalibrationEvaluation::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == LIDAR_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
    return 1;
  }
  if (port == TRAJ_INPUT_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkCalibrationEvaluation::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkCalibrationEvaluation::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkTable* tableOutput = vtkTable::GetData(outputVector, 0);

  vtkSmartPointer<vtkTransform> upstreamT =
    FindUpstreamSensorTransform(this->GetInputConnection(LIDAR_INPUT_PORT, 0));
  double basePos[3] = { 0, 0, 0 };
  double baseOri[3] = { 0, 0, 0 };
  if (upstreamT)
  {
    upstreamT->GetPosition(basePos);
    upstreamT->GetOrientation(baseOri);
    vtkLog(INFO,
      "Upstream SensorTransform: pos=(" << basePos[0] << ", " << basePos[1] << ", " << basePos[2]
                                        << ") orientRPY=(" << baseOri[0] << ", " << baseOri[1]
                                        << ", " << baseOri[2] << ")");
  }
  else
  {
    vtkLog(INFO, "Upstream SensorTransform: not found (using identity)");
    upstreamT.TakeReference(vtkTransform::New());
    upstreamT->Identity();
  }
  vtkSmartPointer<vtkDoubleArray> colRoll = vtkSmartPointer<vtkDoubleArray>::New();
  colRoll->SetName("roll_deg");
  vtkSmartPointer<vtkDoubleArray> colPitch = vtkSmartPointer<vtkDoubleArray>::New();
  colPitch->SetName("pitch_deg");
  vtkSmartPointer<vtkDoubleArray> colYaw = vtkSmartPointer<vtkDoubleArray>::New();
  colYaw->SetName("yaw_deg");
  vtkSmartPointer<vtkDoubleArray> colTx = vtkSmartPointer<vtkDoubleArray>::New();
  colTx->SetName("tx_m");
  vtkSmartPointer<vtkDoubleArray> colTy = vtkSmartPointer<vtkDoubleArray>::New();
  colTy->SetName("ty_m");
  vtkSmartPointer<vtkDoubleArray> colTz = vtkSmartPointer<vtkDoubleArray>::New();
  colTz->SetName("tz_m");
  vtkSmartPointer<vtkDoubleArray> colVox = vtkSmartPointer<vtkDoubleArray>::New();
  colVox->SetName("voxels");
  vtkSmartPointer<vtkDoubleArray> colRank = vtkSmartPointer<vtkDoubleArray>::New();
  colRank->SetName("voxels_rank");

  auto linspace = [](double range, int steps)
  {
    std::vector<double> vals;
    if (range <= 1e-12 || steps <= 1)
    {
      vals.push_back(0.0);
      return vals;
    }
    vals.reserve(steps);
    double start = -range;
    double inc = (2.0 * range) / static_cast<double>(steps - 1);
    for (int k = 0; k < steps; ++k)
    {
      vals.push_back(start + k * inc);
    }
    return vals;
  };

  // Build value lists from ranges/steps; steps <= 1 or range ~0 means no sweep on that axis
  std::vector<double> dX = linspace(this->TranslationRangeM[0], this->TranslationSteps[0]);
  std::vector<double> dY = linspace(this->TranslationRangeM[1], this->TranslationSteps[1]);
  std::vector<double> dZ = linspace(this->TranslationRangeM[2], this->TranslationSteps[2]);
  std::vector<double> dRoll = linspace(this->AngleRangeDeg[0], this->AngleSteps[0]);
  std::vector<double> dPitch = linspace(this->AngleRangeDeg[1], this->AngleSteps[1]);
  std::vector<double> dYaw = linspace(this->AngleRangeDeg[2], this->AngleSteps[2]);

  bool hasAnySweep = (dX.size() > 1) || (dY.size() > 1) || (dZ.size() > 1) || (dRoll.size() > 1) ||
    (dPitch.size() > 1) || (dYaw.size() > 1);
  if (!hasAnySweep)
  {
    dX = { 0.0 };
    dY = { 0.0 };
    dZ = { 0.0 };
    dRoll = { 0.0 };
    dPitch = { 0.0 };
    dYaw = { 0.0 };
  }

  // Compute total number of candidates for progress display
  const long long total = static_cast<long long>(dX.size()) * static_cast<long long>(dY.size()) *
    static_cast<long long>(dZ.size()) * static_cast<long long>(dRoll.size()) *
    static_cast<long long>(dPitch.size()) * static_cast<long long>(dYaw.size());

  long long progress = 0;

  // Nested sweep, each candidate uses a fresh aggregator and transform filter
  for (double vDx : dX)
  {
    for (double vDy : dY)
    {
      for (double vDz : dZ)
      {
        for (double vDr : dRoll)
        {
          for (double vDp : dPitch)
          {
            for (double vDyaw : dYaw)
            {
              // Build candidate absolute RPYXYZ = upstream(base) + delta (for logging/table)
              double cand[6] = { baseOri[0] + vDr,
                baseOri[1] + vDp,
                baseOri[2] + vDyaw,
                basePos[0] + vDx,
                basePos[1] + vDy,
                basePos[2] + vDz };

              // Build delta around upstream center
              vtkNew<vtkTransform> deltaTransform;
              deltaTransform->PreMultiply();
              deltaTransform->Translate(vDx, vDy, vDz);
              deltaTransform->RotateZ(vDyaw);
              deltaTransform->RotateX(vDr);
              deltaTransform->RotateY(vDp);

              vtkNew<vtkTransform> uInv;
              uInv->DeepCopy(upstreamT);
              uInv->Inverse();

              // Apply perturbation in the sensor’s own coordinate frame composedDeltaTransform =
              // upstreamT * deltaTransform * uInv ensures that the rotation and translation occur
              // around the sensor’s center, not around the world origin.
              vtkNew<vtkTransform> composedDeltaTransform;
              composedDeltaTransform->PreMultiply();
              composedDeltaTransform->Concatenate(upstreamT->GetMatrix());
              composedDeltaTransform->Concatenate(deltaTransform->GetMatrix());
              composedDeltaTransform->Concatenate(uInv->GetMatrix());

              vtkNew<vtkTransformPolyDataFilter> tf;
              tf->SetTransform(composedDeltaTransform);
              tf->SetInputConnection(this->GetInputConnection(LIDAR_INPUT_PORT, 0));

              // Set up the offline aggregator to combine the LiDAR input with the INS trajectory.
              vtkNew<vtkAggregatePointsFromTrajectoryOffline> aggregator;
              aggregator->SetAutoDetectTimeArray(this->AutoDetectTimeArray);
              aggregator->SetIsVoxelGridFilterUsed(this->IsVoxelGridFilterUsed);
              aggregator->SetAutoComputeBounds(this->AutoComputeBounds);
              aggregator->SetVoxelSamplingMode(this->VoxelSamplingMode);
              aggregator->SetVoxelLeafSize(this->VoxelLeafSize);
              aggregator->SetAutoDetectTimeUnitConversion(this->AutoDetectTimeUnitConversion);
              aggregator->SetCustomConversionFactorToSecond(this->CustomConversionFactorToSecond);
              aggregator->SetTimeOffset(this->TimeOffset);
              aggregator->SetInterpolationType(this->InterpolationType);
              aggregator->SetAllFrames(this->AllFrames);
              aggregator->SetFirstFrame(this->FirstFrame);
              aggregator->SetLastFrame(this->LastFrame);
              aggregator->SetStepSize(this->StepSize);
              aggregator->SetInputConnection(0, tf->GetOutputPort());
              aggregator->SetInputConnection(1, this->GetInputConnection(TRAJ_INPUT_PORT, 0));

              // Execute once per candidate
              aggregator->Update();
              vtkPolyData* last = aggregator->GetOutput();
              vtkIdType voxels = last ? last->GetNumberOfPoints() : 0;
              ++progress;
              vtkLog(INFO,
                "[" << progress << "/" << total << "] "
                    << "Occupied voxels: " << voxels << ", RPYXYZ= " << cand[0] << ", " << cand[1]
                    << ", " << cand[2] << ", " << cand[3] << ", " << cand[4] << ", " << cand[5]);

              // Append a row to the table columns (rank computed after loop)
              colRoll->InsertNextValue(cand[0]);
              colPitch->InsertNextValue(cand[1]);
              colYaw->InsertNextValue(cand[2]);
              colTx->InsertNextValue(cand[3]);
              colTy->InsertNextValue(cand[4]);
              colTz->InsertNextValue(cand[5]);
              colVox->InsertNextValue(static_cast<double>(voxels));
            }
          }
        }
      }
    }
  }

  // Compute voxels ranking (ascending: smallest voxels => rank 1)
  {
    vtkIdType n = colVox->GetNumberOfTuples();
    std::vector<std::pair<double, vtkIdType>> order;
    order.reserve(static_cast<size_t>(n));
    for (vtkIdType i = 0; i < n; ++i)
    {
      order.emplace_back(colVox->GetValue(i), i);
    }
    std::stable_sort(
      order.begin(), order.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<double> rank(static_cast<size_t>(n), 0.0);
    for (vtkIdType i = 0; i < n; ++i)
    {
      rank[static_cast<size_t>(order[static_cast<size_t>(i)].second)] = static_cast<double>(i + 1);
    }
    colRank->SetNumberOfTuples(n);
    for (vtkIdType i = 0; i < n; ++i)
    {
      colRank->SetValue(i, rank[static_cast<size_t>(i)]);
    }
  }

  tableOutput->AddColumn(colRank);
  tableOutput->AddColumn(colRoll);
  tableOutput->AddColumn(colPitch);
  tableOutput->AddColumn(colYaw);
  tableOutput->AddColumn(colTx);
  tableOutput->AddColumn(colTy);
  tableOutput->AddColumn(colTz);
  tableOutput->AddColumn(colVox);

  return 1;
}
