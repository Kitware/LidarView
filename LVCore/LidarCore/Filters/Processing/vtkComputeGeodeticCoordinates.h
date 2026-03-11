/*=========================================================================

  Program: LidarView
  Module:  vtkComputeGeodeticCoordinates.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkComputeGeodeticCoordinates_h
#define vtkComputeGeodeticCoordinates_h

#include <string>
#include <vtkPolyDataAlgorithm.h>

#include "lvFiltersProcessingModule.h" // For export macro

/**
 * Compute geodetic coordinates (Longitude, Latitude, Altitude) from input XYZ positions
 * by projecting from an input CRS to an output CRS using PROJ.
 */
class LVFILTERSPROCESSING_EXPORT vtkComputeGeodeticCoordinates : public vtkPolyDataAlgorithm
{
public:
  static vtkComputeGeodeticCoordinates* New();
  vtkTypeMacro(vtkComputeGeodeticCoordinates, vtkPolyDataAlgorithm)

  //@{
  /**
   * CRS identifiers (e.g., "EPSG:4978", "EPSG:4326"). Used only when
   * computing Longitude/Latitude/Altitude.
   */
  vtkSetStdStringFromCharMacro(InputCRS);
  vtkGetCharFromStdStringMacro(InputCRS);
  vtkSetStdStringFromCharMacro(OutputCRS);
  vtkGetCharFromStdStringMacro(OutputCRS);
  //@}

protected:
  vtkComputeGeodeticCoordinates() = default;
  ~vtkComputeGeodeticCoordinates() = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkComputeGeodeticCoordinates(const vtkComputeGeodeticCoordinates&) = delete;
  void operator=(const vtkComputeGeodeticCoordinates&) = delete;

  std::string InputCRS;
  std::string OutputCRS;
};

#endif // vtkComputeGeodeticCoordinates_h
