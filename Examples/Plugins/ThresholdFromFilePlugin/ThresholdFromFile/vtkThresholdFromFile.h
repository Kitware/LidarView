/*=========================================================================

  Program: LidarView
  Module:  vtkThresholdFromFile.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkThresholdFromFile_h
#define vtkThresholdFromFile_h

#include <vtkPolyDataAlgorithm.h>

#include <vtkSetGet.h>

#include <string>
#include <vector>

/**
 * vtkThresholdFromFile reads values from a CSV file, selects a specific array
 * within the input, and includes or excludes any points whose values match those in the CSV.
 * A tolerance value can be applied.
 */
class vtkThresholdFromFile : public vtkPolyDataAlgorithm
{
public:
  static vtkThresholdFromFile* New();
  vtkTypeMacro(vtkThresholdFromFile, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Name of the CSV file from which values are extracted.
   * The file should only contain a single column.
   */
  vtkGetMacro(FileName, std::string);
  vtkSetMacro(FileName, std::string);
  ///@}

  ///@{
  /**
   * Name of the scalar array to apply the filter on.
   */
  vtkGetMacro(ActiveScalar, std::string);
  vtkSetMacro(ActiveScalar, std::string);
  ///@}

  ///@{
  /**
   * Tolerance applied in value comparison.
   * Defaults to 0.
   */
  vtkGetMacro(Tolerance, double);
  vtkSetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Inverts the filter result if set to true. Defaults to false.
   */
  vtkGetMacro(InvertResult, bool);
  vtkSetMacro(InvertResult, bool);
  ///@}

protected:
  vtkThresholdFromFile();
  ~vtkThresholdFromFile() override;

  /**
   * RequestInformation is called when a property changes, typically upon clicking
   * the "Apply" button. This method parses the CSV file, populates the FilterValues vector,
   * and performs error checking.
   */
  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * RequestData generates the filter's output, triggered when the VTK pipeline
   * requires an update (e.g., input data changes or a new timestep).
   * The main filtering process is executed here.
   */
  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkThresholdFromFile(const vtkThresholdFromFile&); // Not implemented.
  void operator=(const vtkThresholdFromFile&);       // Not implemented.

  std::string FileName;
  std::string ActiveScalar;
  double Tolerance = 0;
  bool InvertResult = false;
  std::vector<double> FilterValues;
};

#endif
