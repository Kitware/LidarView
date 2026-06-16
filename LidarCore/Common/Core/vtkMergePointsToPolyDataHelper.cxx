/*=========================================================================

  Program:   LidarView
  Module:    vtkMergePointsToPolyDataHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// VTK includes
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

// Local includes
#include "vtkMergePointsToPolyDataHelper.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMergePointsToPolyDataHelper)

//------------------------------------------------------------------------------
bool vtkMergePointsToPolyDataHelper::AddPoint(vtkPolyData* points,
  vtkIdType id,
  const double coord[3])
{
  if (points->GetNumberOfPoints() < id)
  {
    vtkWarningMacro("vtkMergePointsToPolyDataHelper::AddPoint : invalid point id");
    return false;
  }

  if (!this->OutputInitialized)
  {
    this->InitializeData();
    this->Output->GetPointData()->CopyAllocate(points->GetPointData());
    this->OutputInitialized = true;
  }

  if (!this->ResizeData())
  {
    return false;
  }

  // Insert the current point in the output
  this->Output->GetPoints()->InsertNextPoint(coord);
  vtkIdType pointId = this->Output->GetPoints()->GetNumberOfPoints() - 1;
  this->Output->GetVerts()->InsertNextCell(1, &pointId);
  this->Output->GetPointData()->CopyData(points->GetPointData(), pointId, 1, id);
  return true;
}

//------------------------------------------------------------------------------
bool vtkMergePointsToPolyDataHelper::AddPoint(vtkPolyData* points, vtkIdType id)
{
  double coord[3];
  points->GetPoint(id, coord);
  return this->AddPoint(points, id, coord);
}

//------------------------------------------------------------------------------
void vtkMergePointsToPolyDataHelper::InitializeData()
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> cells;
  points->SetDataTypeToDouble();
  points->Resize(this->InitialNumberOfPoints);
  cells->AllocateEstimate(this->InitialNumberOfPoints, 1);
  this->Output->SetPoints(points);
  this->Output->SetVerts(cells);

  this->CurrentDataSize = this->InitialNumberOfPoints;
}

//------------------------------------------------------------------------------
void vtkMergePointsToPolyDataHelper::SetInitialNumberOfPoints(int nbPoints)
{
  this->InitialNumberOfPoints = nbPoints;
}

//------------------------------------------------------------------------------
bool vtkMergePointsToPolyDataHelper::ResizeData()
{
  if (this->Output->GetPoints()->GetNumberOfPoints() > this->CurrentDataSize)
  {
    // Resize the data
    if (!this->Output->GetPoints()->Resize(this->CurrentDataSize + ResizeNumberOfPoints))
    {
      vtkErrorMacro("vtkMergePointsToPolyDataHelper::ResizeData : failed to resize the data");
      return false;
    }
    this->Output->GetVerts()->AllocateEstimate(this->CurrentDataSize + ResizeNumberOfPoints, 1);
    this->CurrentDataSize += this->ResizeNumberOfPoints;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkMergePointsToPolyDataHelper::FreeUnusedMemory()
{
  if (this->OutputInitialized)
  {
    this->Output->GetPoints()->Resize(this->Output->GetPoints()->GetNumberOfPoints());
    this->Output->GetVerts()->Squeeze();
    this->Output->GetPointData()->Squeeze();
  }
}

//------------------------------------------------------------------------------
void vtkMergePointsToPolyDataHelper::Clear()
{
  this->OutputInitialized = false;
  this->Output->Initialize();
}
