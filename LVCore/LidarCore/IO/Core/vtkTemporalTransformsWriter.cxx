/*=========================================================================

  Program: LidarView
  Module:  vtkTemporalTransformsWriter.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTemporalTransformsWriter.h"

#include <iostream>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include "vtkTemporalTransforms.h"

#include "vtkConversions.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTemporalTransformsWriter)

//-----------------------------------------------------------------------------
vtkTemporalTransformsWriter::~vtkTemporalTransformsWriter()
{
  delete[] this->FileName;
}

//-----------------------------------------------------------------------------
int vtkTemporalTransformsWriter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPolyData* polyData = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!polyData)
  {
    vtkErrorMacro("Input is not a vtkPolyData");
    return 0;
  }

  vtkDataArray* orientationArray = this->GetInputArrayToProcess(0, inputVector);
  if (!orientationArray)
  {
    vtkErrorMacro("Could not find orientation array name.");
    return 0;
  }
  auto transforms =
    vtkTemporalTransforms::CreateFromPolyData(polyData, orientationArray->GetName());
  if (!transforms)
  {
    vtkErrorMacro("Input is not a vtkTemporalTransforms");
    return 0;
  }

  vtkDataArray* time = transforms->GetTimeArray();
  // TODO: when we support reading a CSV with commented line, add such comment:
  // # Pose trajectory format, time in s, angles in degree, position in meters
  // # Recompose the rotation part of the pose using:
  // # R = Rz(yaw) * Ry(pitch) * Rx(roll)
  // # a point exprimed in the Lidar reference frame can be exprimed in a fixed
  // # reference frame using: X_fixed = R(t) * X_lidar + [x(t), y(t), z(t)]^T

  // Optional geodetic arrays present on input
  vtkDataArray* lonArray = polyData->GetPointData()->GetArray("Longitude");
  vtkDataArray* latArray = polyData->GetPointData()->GetArray("Latitude");
  vtkDataArray* altArray = polyData->GetPointData()->GetArray("Altitude");

  const vtkIdType npts = transforms->GetNumberOfPoints();
  const bool hasGeodeticCoord = (lonArray && latArray && altArray &&
                          lonArray->GetNumberOfTuples() == npts &&
                          latArray->GetNumberOfTuples() == npts &&
                          altArray->GetNumberOfTuples() == npts);

  std::ofstream file(this->FileName);
  file << "Time,Rx(Roll),Ry(Pitch),Rz(Yaw),X,Y,Z";
  if (hasGeodeticCoord)
  {
    file << ",Longitude,Latitude,Altitude";
  }
  file << endl;

  for (unsigned int i = 0; i < transforms->GetNumberOfPoints(); i++)
  {
    double t = time->GetTuple1(i);
    auto transform = transforms->GetTransform(i);
    std::pair<Eigen::Vector3d, Eigen::Vector3d> towrite = GetPoseParamsFromTransform(transform);
    // We *need* to take care than the user might want to export a trajectory
    // that was produced by projecting GPS coordinate into UTM, thus giving huge
    // coordinates (thousands of kilometers).
    // So make sure that you always show up to N decimals after the point.
    // (not just N significant digits because many digits could be before the
    // point).
    // Possible optimization: hide useless trailing zeros which take up space.
    // (at the cost of breaking the columns alignement that helps a human parse)
    // Write orientation
    if (this->WriteInDegrees)
    {
      file << std::fixed << std::setprecision(17) << t << ","
           << vtkMath::DegreesFromRadians(towrite.first[0]) << ","
           << vtkMath::DegreesFromRadians(towrite.first[1]) << ","
           << vtkMath::DegreesFromRadians(towrite.first[2]);
    }
    else
    {
      file << std::fixed << std::setprecision(17) << t << "," << towrite.first[0] << ","
           << towrite.first[1] << "," << towrite.first[2];
    }
    file << "," << towrite.second[0] << "," << towrite.second[1] << "," << towrite.second[2];

    if (hasGeodeticCoord)
    {
      const double lon = lonArray->GetComponent(i, 0);
      const double lat = latArray->GetComponent(i, 0);
      const double alt = altArray->GetComponent(i, 0);
      file << "," << lon << "," << lat << "," << alt;
    }

    file << std::endl;
  }

  file.close();
  return 1;
}
