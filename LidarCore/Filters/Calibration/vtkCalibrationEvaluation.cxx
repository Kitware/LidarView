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
#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCallbackCommand.h>
#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLidarPacketInterpreter.h>
#include <vtkLidarReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

vtkStandardNewMacro(vtkCalibrationEvaluation)
namespace
{
constexpr unsigned int LIDAR_INPUT_PORT = 0;
constexpr unsigned int TRAJ_INPUT_PORT = 1;
constexpr unsigned int INPUT_PORT_COUNT = 2;

constexpr unsigned int TABLE_OUTPUT_PORT = 0;
constexpr unsigned int AGGREGATED_CLOUD_OUTPUT_PORT = 1;
constexpr unsigned int OUTPUT_PORT_COUNT = 2;

//-----------------------------------------------------------------------------
// Compute voxels ranking (ascending => rank 1)
void ComputeVoxelRanking(vtkDoubleArray* voxels, vtkDoubleArray* outRank)
{
  if (!voxels || !outRank)
  {
    return;
  }
  vtkIdType n = voxels->GetNumberOfTuples();
  std::vector<std::pair<double, vtkIdType>> order;
  order.reserve(static_cast<size_t>(n));
  for (vtkIdType i = 0; i < n; ++i)
  {
    order.emplace_back(voxels->GetValue(i), i);
  }
  std::stable_sort(
    order.begin(), order.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

  outRank->SetNumberOfTuples(n);
  for (vtkIdType i = 0; i < n; ++i)
  {
    vtkIdType idx = order[static_cast<size_t>(i)].second;
    outRank->SetValue(idx, static_cast<double>(i + 1));
  }
}

//-----------------------------------------------------------------------------
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
int vtkCalibrationEvaluation::RequestInformation(vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector*)
{
  // Discover upstream sensor transform for information-only UI exposure
  vtkSmartPointer<vtkTransform> upstreamT =
    FindUpstreamSensorTransform(this->GetInputConnection(LIDAR_INPUT_PORT, 0));
  if (upstreamT)
  {
    double pos[3] = { 0., 0., 0. };
    double ori[3] = { 0., 0., 0. };
    upstreamT->GetPosition(pos);
    upstreamT->GetOrientation(ori);
    this->UpstreamFound = true;
    this->UpstreamPosition[0] = pos[0];
    this->UpstreamPosition[1] = pos[1];
    this->UpstreamPosition[2] = pos[2];
    this->UpstreamRotationDeg[0] = ori[0];
    this->UpstreamRotationDeg[1] = ori[1];
    this->UpstreamRotationDeg[2] = ori[2];
  }
  else
  {
    this->UpstreamFound = false;
  }
  this->Modified();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCalibrationEvaluation::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkTable* tableOutput = vtkTable::GetData(outputVector, 0);
  // Choose sweep center: upstream pose if enabled and available; otherwise user base pose.
  double basePos[3];
  double baseOri[3];
  if (this->UseUpstreamCalibration && this->UpstreamFound)
  {
    basePos[0] = this->UpstreamPosition[0];
    basePos[1] = this->UpstreamPosition[1];
    basePos[2] = this->UpstreamPosition[2];
    baseOri[0] = this->UpstreamRotationDeg[0];
    baseOri[1] = this->UpstreamRotationDeg[1];
    baseOri[2] = this->UpstreamRotationDeg[2];
  }
  else
  {
    basePos[0] = this->BasePosition[0];
    basePos[1] = this->BasePosition[1];
    basePos[2] = this->BasePosition[2];
    baseOri[0] = this->BaseRotationDeg[0];
    baseOri[1] = this->BaseRotationDeg[1];
    baseOri[2] = this->BaseRotationDeg[2];
  }

  vtkNew<vtkTransform> centerT;
  centerT->PreMultiply();
  centerT->RotateZ(baseOri[2]);
  centerT->RotateX(baseOri[0]);
  centerT->RotateY(baseOri[1]);
  centerT->Translate(basePos);
  vtkLog(INFO,
    "Sweep center: pos=("
      << basePos[0] << ", " << basePos[1] << ", " << basePos[2] << ") orientRPY=(" << baseOri[0]
      << ", " << baseOri[1] << ", " << baseOri[2] << ")"
      << (this->UseUpstreamCalibration && this->UpstreamFound ? " [upstream]" : " [user base]"));
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

  vtkSmartPointer<vtkTransform> inputT =
    FindUpstreamSensorTransform(this->GetInputConnection(LIDAR_INPUT_PORT, 0));
  vtkNew<vtkTransform> inputInv;
  inputInv->Identity();
  if (inputT)
  {
    inputInv->DeepCopy(inputT);
    inputInv->Inverse();
  }

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

  // Require aggregator to be provided as a subproxy
  vtkAggregatePointsFromTrajectoryOffline* aggregatorPtr = this->Aggregator;

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

              // Unified application: M = T0 · Δ · (T_in)^-1
              vtkNew<vtkTransform> composedTransform;
              composedTransform->PreMultiply();
              composedTransform->Concatenate(centerT->GetMatrix());
              composedTransform->Concatenate(deltaTransform->GetMatrix());
              composedTransform->Concatenate(inputInv->GetMatrix());

              vtkNew<vtkTransformPolyDataFilter> tf;
              tf->SetTransform(composedTransform);
              tf->SetInputConnection(this->GetInputConnection(LIDAR_INPUT_PORT, 0));

              // Wire inputs and Clear previous state
              aggregatorPtr->Clear();
              aggregatorPtr->SetInputConnection(0, tf->GetOutputPort());
              aggregatorPtr->SetInputConnection(1, this->GetInputConnection(TRAJ_INPUT_PORT, 0));

              // Execute once per candidate
              aggregatorPtr->Update();
              vtkPolyData* last = aggregatorPtr->GetOutput();
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
  ComputeVoxelRanking(colVox, colRank);

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

//-----------------------------------------------------------------------------
void vtkCalibrationEvaluation::SetBasePosition(double x, double y, double z)
{
  if (this->BasePosition[0] == x && this->BasePosition[1] == y && this->BasePosition[2] == z)
  {
    return;
  }
  this->BasePosition[0] = x;
  this->BasePosition[1] = y;
  this->BasePosition[2] = z;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCalibrationEvaluation::SetBaseRotationDeg(double roll, double pitch, double yaw)
{
  if (this->BaseRotationDeg[0] == roll && this->BaseRotationDeg[1] == pitch &&
    this->BaseRotationDeg[2] == yaw)
  {
    return;
  }
  this->BaseRotationDeg[0] = roll;
  this->BaseRotationDeg[1] = pitch;
  this->BaseRotationDeg[2] = yaw;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCalibrationEvaluation::SetAggregator(vtkAggregatePointsFromTrajectoryOffline* agg)
{
  if (this->Aggregator == agg)
  {
    return;
  }
  // Remove previous observer if any
  if (this->Aggregator && this->AggregatorObserverTag)
  {
    this->Aggregator->RemoveObserver(this->AggregatorObserverTag);
    this->AggregatorObserverTag = 0;
  }

  this->Aggregator = agg;

  vtkNew<vtkCallbackCommand> cb;
  cb->SetClientData(this);
  cb->SetCallback(
    [](vtkObject*, unsigned long, void* clientData, void*)
    {
      auto self = reinterpret_cast<vtkCalibrationEvaluation*>(clientData);
      self->Modified();
    });
  this->AggregatorObserverTag = this->Aggregator->AddObserver(vtkCommand::ModifiedEvent, cb);

  this->Modified();
}
