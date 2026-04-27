#include "vtkINSTableToPoses.h"
#include "vtkNormalizeExternalSensorData.h"

#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTestUtilities.h>
#include <vtkTransform.h>

#include <iostream>
#include <string>

namespace
{
constexpr const char* COLUMN_NAMES[] = { "timestamp", "x", "y", "z", "roll", "pitch", "heading" };
constexpr double INS_DATA[5][7] = {
  { 1000.000, 48.8566, 2.3522, 35.0, 0.10, -0.20, 45.00 },
  { 1000.100, 48.8567, 2.3523, 35.1, 0.11, -0.21, 45.10 },
  { 1000.200, 48.8568, 2.3524, 35.2, 0.12, -0.22, 45.20 },
  { 1000.300, 48.8569, 2.3525, 35.3, 0.13, -0.23, 45.30 },
  { 1000.400, 48.8570, 2.3526, 35.4, 0.14, -0.24, 45.40 },
};
constexpr double EPSILON = 1e-6;

//------------------------------------------------------------------------------
static vtkSmartPointer<vtkTable> BuildSyntheticINSTable()
{
  vtkNew<vtkTable> table;

  for (int col = 0; col < 7; ++col)
  {
    vtkNew<vtkDoubleArray> array;
    array->SetName(::COLUMN_NAMES[col]);
    array->SetNumberOfValues(5);
    for (int row = 0; row < 5; ++row)
    {
      array->SetValue(row, ::INS_DATA[row][col]);
    }
    table->AddColumn(array);
  }
  return table;
}
}

//------------------------------------------------------------------------------
int TestINSTableToPoses(int, char*[])
{
  vtkSmartPointer<vtkTable> data = ::BuildSyntheticINSTable();

  vtkNew<vtkTransform> transform;
  transform->Translate(5, 0, -2);
  transform->RotateX(5);
  transform->RotateY(-3);

  vtkNew<vtkNormalizeExternalSensorData> normalizer;
  normalizer->SetInputData(data);
  normalizer->SetTimeColumn(COLUMN_NAMES[0]);
  normalizer->SetGNSSXColumn(COLUMN_NAMES[1]);
  normalizer->SetGNSSYColumn(COLUMN_NAMES[2]);
  normalizer->SetGNSSZColumn(COLUMN_NAMES[3]);
  normalizer->SetRollColumn(COLUMN_NAMES[4]);
  normalizer->SetPitchColumn(COLUMN_NAMES[5]);
  normalizer->SetYawColumn(COLUMN_NAMES[6]);
  normalizer->SetSensorTransform(transform);
  normalizer->SetUseINS(true);

  vtkNew<vtkINSTableToPoses> converter;
  converter->SetInputConnection(normalizer->GetOutputPort());
  converter->Update();

  vtkPolyData* polyData = vtkPolyData::SafeDownCast(converter->GetOutput());
  if (!polyData)
  {
    std::cerr << "vtkINSTableToPoses output is a vtkPolyData" << std::endl;
    return EXIT_FAILURE;
  }
  if (polyData->GetNumberOfPoints() != 5)
  {
    std::cerr << "PolyData doesn't have the expected number of points" << std::endl;
    return EXIT_FAILURE;
  }

  for (int i = 0; i < 5; ++i)
  {
    double point[3];
    polyData->GetPoint(i, point);

    double original[4] = { ::INS_DATA[i][1], ::INS_DATA[i][2], ::INS_DATA[i][3], 1.0 };
    double expected[4];
    transform->GetMatrix()->MultiplyPoint(original, expected);

    if (std::abs(point[0] - expected[0]) > ::EPSILON ||
      std::abs(point[1] - expected[1]) > ::EPSILON || std::abs(point[2] - expected[2]) > ::EPSILON)
    {
      std::cerr << "Point " << i << " is not correctly transformed.\n"
                << "  Original: " << original[0] << ", " << original[1] << ", " << original[2]
                << "\n"
                << "  Expected: " << expected[0] << ", " << expected[1] << ", " << expected[2]
                << "\n"
                << "  Got:      " << point[0] << ", " << point[1] << ", " << point[2] << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
