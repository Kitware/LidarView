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
#include "vtkLidarPosePacketInterpreter.h"

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

// This is the "real" implementation of PoseLidarStream
class vtkLidarPoseStream::vtkInternals : public vtkStream
{
public:
  //-----------------------------------------------------------------------------
  vtkInternals()
  {
    this->AllPoses = vtkSmartPointer<vtkPolyData>::New();
    this->AllRawInformation = vtkSmartPointer<vtkTable>::New();
    this->LastNumberPositionOrientationInformation = 0;
    this->LastNumberRawInformation = 0;

    this->SetListeningPort(8308);
    this->SetForwardedPort(8308);
  }

  //-----------------------------------------------------------------------------
  ~vtkInternals() override { this->Stop(); }

  //-----------------------------------------------------------------------------
  void AddNewData() override
  {
    if (this->GetPoseInterpreter()->HasRawInformation())
    {
      if (this->AllRawInformation->GetNumberOfRows() == 0)
      {
        this->AllRawInformation->DeepCopy(this->GetPoseInterpreter()->GetRawInformation());
        return;
      }

      vtkSmartPointer<vtkTable> raw = this->GetPoseInterpreter()->GetRawInformation();

      for (int i = 0; i < raw->GetNumberOfRows(); i++)
      {
        vtkVariantArray* array = raw->GetRow(i);
        this->AllRawInformation->InsertNextRow(array);
      }
    }

    if (this->GetPoseInterpreter()->HasPoseInformation())
    {
      if (this->AllPoses->GetNumberOfPoints() == 0)
      {
        this->AllPoses->DeepCopy(this->GetPoseInterpreter()->GetPose());
        return;
      }
      // Copying the new pose information (points, rows, ...)
      // available in the interpreter to the corresponding buffer
      vtkSmartPointer<vtkPolyData> pose = this->GetPoseInterpreter()->GetPose();
      vtkPoints* points = this->AllPoses->GetPoints();
      for (vtkIdType i = 0; i < pose->GetNumberOfPoints(); i++)
      {
        for (vtkIdType j = 0; j < pose->GetPointData()->GetNumberOfArrays(); j++)
        {
          // Insert the i-th tuple of the current array (source) to the corresponding array of the
          // bufferize vtkPolyData
          vtkAbstractArray* source = pose->GetPointData()->GetAbstractArray(j);
          this->AllPoses->GetPointData()->GetAbstractArray(j)->InsertNextTuple(i, source);
        }
        // Update points
        double pointToAdd[3];
        pose->GetPoint(i, pointToAdd);
        points->InsertNextPoint(pointToAdd);
        this->AllPoses->SetPoints(points);
      }

      // Set the polyline to the poly data to see the pose information
      vtkSmartPointer<vtkPolyLine> polyLine = CreatePolyLineFromPoints(this->AllPoses->GetPoints());
      vtkNew<vtkCellArray> cellArray;
      cellArray->InsertNextCell(polyLine);
      this->AllPoses->SetLines(cellArray);
    }
  }

  //-----------------------------------------------------------------------------
  void ClearAllDataAvailable() override { this->GetPoseInterpreter()->ResetCurrentData(); }

  //-----------------------------------------------------------------------------
  int CheckForNewData() override
  {
    return this->CheckNewDataPositionOrientation() + this->CheckForNewDataRawInformation();
  }

  //-----------------------------------------------------------------------------
  vtkLidarPosePacketInterpreter* GetPoseInterpreter()
  {
    return vtkLidarPosePacketInterpreter::SafeDownCast(this->Interpreter);
  }

  //-----------------------------------------------------------------------------
  void SetPoseInterpreter(vtkLidarPosePacketInterpreter* interpreter)
  {
    this->Interpreter = interpreter;
  }

  //-----------------------------------------------------------------------------
  virtual int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector) override
  {
    {
      std::lock_guard<std::mutex> lock(this->DataMutex);

      vtkPolyData* outputPositionsOrientations =
        vtkPolyData::GetData(outputVector, ::LIDAR_POSE_PORT);
      outputPositionsOrientations->DeepCopy(this->AllPoses);
      this->LastNumberPositionOrientationInformation = this->AllPoses->GetNumberOfPoints();

      vtkTable* outputRawInformation = vtkTable::GetData(outputVector, ::POSE_RAW_INFORMATION_PORT);
      ::DeepReverseCopy(this->AllRawInformation, outputRawInformation);
      this->LastNumberRawInformation = this->AllRawInformation->GetNumberOfRows();
    }
    return 1;
  };

private:
  vtkInternals(const vtkInternals&) = delete;
  void operator=(const vtkInternals&) = delete;

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

public:
  vtkSmartPointer<vtkPolyData> AllPoses;
  vtkSmartPointer<vtkTable> AllRawInformation;

  unsigned int LastNumberPositionOrientationInformation;
  unsigned int LastNumberRawInformation;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLidarPoseStream)

//-----------------------------------------------------------------------------
vtkLidarPoseStream::vtkLidarPoseStream()
  : Internals(new vtkLidarPoseStream::vtkInternals())
{
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
  int ret = this->Internals->RequestData(request, inputVector, outputVector);
  return ret && vtkLidarStream::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
vtkLidarPosePacketInterpreter* vtkLidarPoseStream::GetPoseInterpreter()
{
  return this->Internals->GetPoseInterpreter();
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetPoseInterpreter(vtkLidarPosePacketInterpreter* interpreter)
{
  this->Internals->SetPoseInterpreter(interpreter);
}

//----------------------------------------------------------------------------
int vtkLidarPoseStream::GetGNSSPort()
{
  return this->Internals->GetListeningPort();
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetGNSSPort(int port)
{
  return this->Internals->SetListeningPort(port);
}

//----------------------------------------------------------------------------
int vtkLidarPoseStream::GetGNSSForwardedPort()
{
  return this->Internals->GetForwardedPort();
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetGNSSForwardedPort(int port)
{
  return this->Internals->SetForwardedPort(port);
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetMulticastAddress(const std::string& value)
{
  this->Internals->SetMulticastAddress(value);
  vtkLidarStream::SetMulticastAddress(value);
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetLocalListeningAddress(const std::string& value)
{
  this->Internals->SetLocalListeningAddress(value);
  vtkLidarStream::SetLocalListeningAddress(value);
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetForwardedIpAddress(const std::string& value)
{
  this->Internals->SetForwardedIpAddress(value);
  vtkLidarStream::SetForwardedIpAddress(value);
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetIsForwarding(bool value)
{
  this->Internals->SetIsForwarding(value);
  vtkLidarStream::SetIsForwarding(value);
}

//----------------------------------------------------------------------------
void vtkLidarPoseStream::SetIsCrashAnalysing(bool value)
{
  this->Internals->SetIsCrashAnalysing(value);
  vtkLidarStream::SetIsCrashAnalysing(value);
}

//-----------------------------------------------------------------------------
void vtkLidarPoseStream::Start()
{
  vtkLidarStream::Start();
  this->Internals->Start();
}

//-----------------------------------------------------------------------------
void vtkLidarPoseStream::Stop()
{
  vtkLidarStream::Stop();
  this->Internals->Stop();
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkLidarPoseStream::GetMTime()
{
  return std::max(this->vtkLidarStream::GetMTime(), this->Internals->GetMTime());
}
