/*=========================================================================

  Program: LidarView
  Module:  vtkTemporalTransformsReader.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef VTKTEMPORALTRANSFORMSREADER_H
#define VTKTEMPORALTRANSFORMSREADER_H

// STD
#include <string>

// VTK
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

// LOCAL
#include "vtkTemporalTransforms.h"

#include "lvIOCoreModule.h"

/**
 * @brief vtkTemporalTransformsReader reads a csv file to generate a vtkTemporalTransform.
 *
 * The CSV file is expected to respect the following specification:
 * - Element separator = ","
 * - Line separator = "\n"
 * - Number of elements >= 7
 * - Header example = "Time,Rx(Roll),Ry(Pitch),Rz(Yaw),X,Y,Z"
 * - Time  : expressed in seconds
 * - Rx(Roll)  : rotation around X axis, in degrees
 * - Ry(Pitch) : rotation around Y axis, in degrees
 * - Rz(Yaw)   : rotation around Z axis, in degrees
 * - The rotation matrix can be recomposed this way: R = Rz(yaw)*Ry(pitch)*Rx(roll)
 *
 * Optional columns: Longitude(d), Latitude(d), Altitude(m)
 *   If arrays named "Longitude", "Latitude", "Altitude" are present,
 *   they are copied into the output as point-data arrays.
 *
 * Remark: if you get from LidarView UI the error:
 * "vtkSIProxyDefinitionManager: No proxy that matches: group= and proxy= were found."
 * when you tried to open a file with a ".csv" extension and was asked to chose
 * between standard "Delimited Text" and "pose trajectory" then it means you
 * did not click on "pose trajectory".
 * This is a bug solved in recent ParaViews.
 * see: https://gitlab.kitware.com/paraview/paraview/issues/17594
 */
class LVIOCORE_EXPORT vtkTemporalTransformsReader : public vtkPolyDataAlgorithm
{
public:
  static vtkTemporalTransformsReader* New();
  vtkTypeMacro(vtkTemporalTransformsReader, vtkPolyDataAlgorithm)

  static vtkSmartPointer<vtkTemporalTransforms> OpenTemporalTransforms(const std::string& filename);

  //@{
  /**
   * @copydoc vtkTemporalTransformsReader::TimeOffset
   */
  vtkGetMacro(TimeOffset, double);
  vtkSetMacro(TimeOffset, double);
  //@}

  //@{
  /**
   * @copydoc vtkTemporalTransformsReader::FileName
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * If true consider orientation angles are in degrees and convert them to radians.
   * Default to false
   */
  vtkSetMacro(ConvertAnglesToRadians, bool);
  vtkGetMacro(ConvertAnglesToRadians, bool);
  //@}

protected:
  vtkTemporalTransformsReader();
  ~vtkTemporalTransformsReader() override;

  //! Read the data from the csv file and fill TrajectoryCahce
  //! can throw an error
  void ReadData();

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  //! TimeOffset in seconds relative to the system clock
  double TimeOffset = 0.0;
  char* FileName;
  bool ConvertAnglesToRadians = false;

  vtkTemporalTransformsReader(const vtkTemporalTransformsReader&) = delete;
  void operator=(const vtkTemporalTransformsReader&) = delete;
};

#endif // VTKTEMPORALTRANSFORMSREADER_H
