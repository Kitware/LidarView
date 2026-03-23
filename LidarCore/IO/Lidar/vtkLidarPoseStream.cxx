/*=========================================================================

  Program: LidarView
  Module:  vtkLidarPoseStream.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLidarPoseStream.h"
#include "vtkHelper.h"
#include "vtkPosePacketInterpreter.h"
#include "vtkStreamPacketHandler.h"

#include <sstream>

#include <vtkCellArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLine.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyLine.h>
#include <vtkVariantArray.h>

namespace
{
void DeepReverseCopy(vtkTable* input, vtkTable* output)
{
  vtkIdType nbCol = input->GetNumberOfColumns();
  vtkIdType nbRow = input->GetNumberOfRows();
  // copy array
  for (vtkIdType i = 0; i < nbCol; ++i)
  {
    auto* original = input->GetColumn(i);

    // handle only case with one element
    if (original->GetNumberOfComponents() != 1)
    {
      vtkGenericWarningMacro(<< "Currently only array with one componenets are supported, "
                             << "this is not the case of array :" << original->GetName());
      continue;
    }

    auto* array = original->NewInstance();
    array->SetName(original->GetName());
    array->SetNumberOfValues(original->GetNumberOfValues());
    for (vtkIdType j = 0; j < nbRow; ++j)
    {
      auto inv = (nbRow - 1) - j;
      array->SetVariantValue(j, original->GetVariantValue(inv));
    }
    output->AddColumn(array);
  }
}

constexpr unsigned int LIDAR_POSE_PORT = 2;
constexpr unsigned int POSE_RAW_INFORMATION_PORT = 3;
}

class vtkLidarPoseStream::vtkInternals
{
public:
  vtkSmartPointer<vtkPolyData> AllPoses;
  vtkSmartPointer<vtkTable> AllRawInformation;

  unsigned int LastNumberPositionOrientationInformation;
  unsigned int LastNumberRawInformation;

  //-----------------------------------------------------------------------------
  int CheckNewDataPositionOrientation()
  {
    return this->AllPoses->GetNumberOfPoints() - this->LastNumberPositionOrientationInformation;
  }

  //-----------------------------------------------------------------------------
  int CheckForNewDataRawInformation()
  {
    return this->AllRawInformation->GetNumberOfRows() - this->LastNumberRawInformation;
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarPoseStream)

//-----------------------------------------------------------------------------
vtkLidarPoseStream::vtkLidarPoseStream()
  : Internals(new vtkLidarPoseStream::vtkInternals())
{
  auto& internals = *this->Internals;
  internals.AllPoses = vtkSmartPointer<vtkPolyData>::New();
  internals.AllRawInformation = vtkSmartPointer<vtkTable>::New();
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(4);
}

//-----------------------------------------------------------------------------
int vtkLidarPoseStream::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == ::LIDAR_POSE_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    return 1;
  }

  if (port == ::POSE_RAW_INFORMATION_PORT)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }
  return vtkLidarStream::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkLidarPoseStream::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  auto& internals = *this->Internals;
  {
    std::lock_guard<std::mutex> lock(this->DataMutex);

    vtkPolyData* outputPositionsOrientations =
      vtkPolyData::GetData(outputVector, ::LIDAR_POSE_PORT);
    outputPositionsOrientations->DeepCopy(internals.AllPoses);
    internals.LastNumberPositionOrientationInformation = internals.AllPoses->GetNumberOfPoints();

    vtkTable* outputRawInformation = vtkTable::GetData(outputVector, ::POSE_RAW_INFORMATION_PORT);
    ::DeepReverseCopy(internals.AllRawInformation, outputRawInformation);
    internals.LastNumberRawInformation = internals.AllRawInformation->GetNumberOfRows();
  }

  return Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::Start()
{
  if (!this->GetLidarInterpreter())
  {
    vtkErrorMacro("No packet interpreter selected.");
    return;
  }
  if (!this->GetLidarInterpreter()->GetIsInitialized())
  {
    this->GetLidarInterpreter()->Initialize();
  }

  this->vtkStream::Start({ this->GetListeningPort(), this->GNSSPort });
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::ConsumePacket(const std::vector<uint8_t>& pkt, double timestamp)
{
  auto interp = this->GetPoseInterpreter();
  if (interp->IsValidPacket(pkt.data(), pkt.size()))
  {
    interp->ProcessPacketWrapped(pkt.data(), pkt.size(), timestamp);
    if (interp->IsNewData())
    {
      {
        std::lock_guard<std::mutex> lock(this->DataMutex);
        this->AddNewData();
      }
      interp->ResetCurrentData();
    }
  }
  Superclass::ConsumePacket(pkt, timestamp);
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::AddNewData()
{
  auto& internals = *this->Internals;
  if (this->GetPoseInterpreter()->HasRawInformation())
  {
    if (internals.AllRawInformation->GetNumberOfRows() == 0)
    {
      internals.AllRawInformation->DeepCopy(this->GetPoseInterpreter()->GetRawInformation());
      return;
    }

    vtkSmartPointer<vtkTable> raw = this->GetPoseInterpreter()->GetRawInformation();

    for (int i = 0; i < raw->GetNumberOfRows(); i++)
    {
      vtkVariantArray* array = raw->GetRow(i);
      internals.AllRawInformation->InsertNextRow(array);
    }
  }

  if (this->GetPoseInterpreter()->HasPoseInformation())
  {
    if (internals.AllPoses->GetNumberOfPoints() == 0)
    {
      internals.AllPoses->DeepCopy(this->GetPoseInterpreter()->GetPose());
      return;
    }
    // Copying the new pose information (points, rows, ...)
    // available in the interpreter to the corresponding buffer
    vtkSmartPointer<vtkPolyData> pose = this->GetPoseInterpreter()->GetPose();
    vtkPoints* points = internals.AllPoses->GetPoints();
    for (vtkIdType i = 0; i < pose->GetNumberOfPoints(); i++)
    {
      for (vtkIdType j = 0; j < pose->GetPointData()->GetNumberOfArrays(); j++)
      {
        // Insert the i-th tuple of the current array (source) to the corresponding array of the
        // bufferize vtkPolyData
        vtkAbstractArray* source = pose->GetPointData()->GetAbstractArray(j);
        internals.AllPoses->GetPointData()->GetAbstractArray(j)->InsertNextTuple(i, source);
      }
      // Update points
      double pointToAdd[3];
      pose->GetPoint(i, pointToAdd);
      points->InsertNextPoint(pointToAdd);
      internals.AllPoses->SetPoints(points);
    }

    // Set the polyline to the poly data to see the pose information
    vtkSmartPointer<vtkPolyLine> polyLine =
      CreatePolyLineFromPoints(internals.AllPoses->GetPoints());
    vtkNew<vtkCellArray> cellArray;
    cellArray->InsertNextCell(polyLine);
    internals.AllPoses->SetLines(cellArray);
  }
}

//----------------------------------------------------------------------------
int vtkLidarPoseStream::CheckForNewData()
{
  auto& internals = *this->Internals;
  return internals.CheckNewDataPositionOrientation() + internals.CheckForNewDataRawInformation() +
    Superclass::CheckForNewData();
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetPoseInterpreter(vtkPosePacketInterpreter* interpreter)
{
  this->PoseInterpreter = interpreter;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkLidarPoseStream::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->PoseInterpreter->GetMTime());
}
