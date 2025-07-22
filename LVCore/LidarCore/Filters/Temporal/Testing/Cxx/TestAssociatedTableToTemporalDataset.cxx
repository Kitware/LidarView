/*=========================================================================

  Program:   LidarView
  Module:    TestTrailingFrame.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <numeric>
#include <vector>

#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkGenerateTimeSteps.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkShortArray.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include "vtkAssociateTableToTemporalDataset.h"

namespace
{
constexpr double expectedValue[] = { 15., 20., 30. };
constexpr const char* dataArrayName = "Data";
constexpr const char* timeArrayName = "Time";
}

vtkSmartPointer<vtkPolyData> CreatePointCloud()
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);

  vtkNew<vtkPolyData> grid;
  grid->SetPoints(points.GetPointer());
  return grid;
}

vtkSmartPointer<vtkTable> CreateTable()
{
  vtkNew<vtkTable> table;

  vtkNew<vtkDoubleArray> col1;
  col1->SetName(::dataArrayName);
  col1->InsertNextValue(10.0);
  col1->InsertNextValue(::expectedValue[0]);
  col1->InsertNextValue(::expectedValue[1]);
  col1->InsertNextValue(::expectedValue[2]);

  vtkNew<vtkDoubleArray> timeArray;
  timeArray->SetName(::timeArrayName);
  timeArray->InsertNextValue(0.0);
  timeArray->InsertNextValue(0.4);
  timeArray->InsertNextValue(1.4);
  timeArray->InsertNextValue(2.9);

  table->AddColumn(col1.GetPointer());
  table->AddColumn(timeArray.GetPointer());
  return table;
}

int TestAssociatedTableToTemporalDataset(int, char*[])
{
  vtkSmartPointer<vtkPolyData> dataset = CreatePointCloud();
  vtkSmartPointer<vtkTable> table = CreateTable();

  vtkNew<vtkGenerateTimeSteps> temporal;
  std::vector<double> timesteps(6);
  std::iota(timesteps.begin(), timesteps.end(), 0);
  temporal->SetTimeStepValues(10, timesteps.data());
  temporal->SetInputData(dataset);
  temporal->Update();

  vtkNew<vtkAssociateTableToTemporalDataset> associateFilter;
  associateFilter->SetInputConnection(0, temporal->GetOutputPort());
  associateFilter->SetInputData(1, table);
  associateFilter->SetTimeArrayName(::timeArrayName);

  for (unsigned int timeIdx = 0; timeIdx < timesteps.size(); timeIdx++)
  {
    associateFilter->UpdateTimeStep(timesteps[timeIdx]);
    associateFilter->Update();

    vtkPolyData* output = vtkPolyData::SafeDownCast(associateFilter->GetOutputDataObject(0));

    vtkFieldData* fd = output->GetFieldData();
    if (!fd)
    {
      std::cerr << "Could not found field data array" << std::endl;
      return 1;
    }

    vtkShortArray* validityArray = vtkShortArray::SafeDownCast(
      fd->GetAbstractArray(vtkAssociateTableToTemporalDataset::VALIDITY_ARRAY_NAME()));
    if (!validityArray)
    {
      std::cerr << "Could not find validity array" << std::endl;
      return 1;
    }
    bool hasData = timeIdx < 3;
    if (validityArray->GetValue(0) != hasData)
    {
      std::cerr << "Invalid value for validity array" << std::endl;
      return 1;
    }

    if (hasData)
    {
      vtkDoubleArray* result = vtkDoubleArray::SafeDownCast(fd->GetAbstractArray(::dataArrayName));
      if (!result)
      {
        std::cerr << ::dataArrayName << " is not in field data array" << std::endl;
        return 1;
      }
      if (result->GetNumberOfValues() != 1)
      {
        std::cerr << "One row is expected in array" << std::endl;
        return 1;
      }
      if (result->GetValue(0) != ::expectedValue[timeIdx])
      {
        std::cerr << "Invalid value: " << result->GetValue(0) << " != " << ::expectedValue[timeIdx]
                  << std::endl;
        return 1;
      }
    }
  }

  return 0;
}
