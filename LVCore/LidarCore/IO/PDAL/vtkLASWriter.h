/*=========================================================================

  Program: LidarView
  Module:  vtkLASWriter.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class vtkLASWriter
 * @brief Write a LAS/LAZ file using PDAL with support for AutoOffset.
 *
 * vtkLASWriter writes LAS (or compressed LAZ) files with a polydata input.
 * It also writes all polydata arrays by default.
 *
 * @note Multiples improvments could be done on this writer:
 * - Implement spatial reference support from the georeference we have
 * with some sensors.
 * - Pass metadata (e.g scale, offset) directly from the .las reader
 * as PDAL suggest to do (with vtkFieldData?)
 */

#ifndef vtkLASWriter_h
#define vtkLASWriter_h

#include "lvIOPDALModule.h"  // For export macro
#include "vtkSmartPointer.h" // For protected ivars
#include "vtkWriter.h"

#include <string> // For string parameter

class vtkPolyData;

class LVIOPDAL_EXPORT vtkLASWriter : public vtkWriter
{
public:
  static vtkLASWriter* New();
  vtkTypeMacro(vtkLASWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkPolyData* GetInput();
  vtkPolyData* GetInput(int port);
  ///@}

  ///@{
  /**
   * Specify file name of vtk polygon data file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * The spatial reference system of the file to be written.
   * Can be an EPSG string (e.g. "EPSG:26910") or a WKT string.
   */
  vtkSetMacro(SRSInformation, std::string);
  vtkGetMacro(SRSInformation, std::string);
  ///@}

  ///@{
  /**
   * Offset to be subtracted from the X, Y and Z nominal values, respectively.
   */
  vtkSetVector3Macro(Offset, double);
  vtkGetVectorMacro(Offset, double, 3);
  ///@}

  ///@{
  /**
   * Apply compression to the output, creating a LAZ file instead of a LAS file.
   */
  vtkSetMacro(Compression, bool);
  vtkGetMacro(Compression, bool);
  ///@}

  ///@{
  /**
   * If true, the writer will compute the offset to apply to the points
   * so that the coordinates are as close to the origin as possible.
   */
  vtkSetMacro(AutoOffset, bool);
  vtkGetMacro(AutoOffset, bool);
  ///@}

protected:
  vtkLASWriter();
  ~vtkLASWriter() override;

  void WriteData() override;

  char* FileName = nullptr;
  std::string SRSInformation;
  bool Compression = false;
  bool AutoOffset = false;
  double Offset[3];

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkLASWriter(const vtkLASWriter&) = delete;
  void operator=(const vtkLASWriter&) = delete;
};

#endif
