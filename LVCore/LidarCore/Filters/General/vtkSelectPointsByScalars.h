/*=========================================================================

  Program: LidarView
  Module:  vtkSelectPointsByScalars.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSelectPointsByScalars_h
#define vtkSelectPointsByScalars_h

// VTK includes
#include <vtkDataSetAlgorithm.h>
#include <vtkSmartPointer.h>

#include "lvFiltersGeneralModule.h" // for export macro

/**
 * Filter that selects points from the input dataset whose scalar values
 * match specified values.
 */

class LVFILTERSGENERAL_EXPORT vtkSelectPointsByScalars : public vtkDataSetAlgorithm
{
public:
  static vtkSelectPointsByScalars* New();

  vtkTypeMacro(vtkSelectPointsByScalars, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set how many scalar values will be used for point selection.
   * Resize the internal container accordingly.
   */
  void SetNumberOfValues(int nbValues);

  /**
   * Set the scalar value at the given index to be used for selection.
   */
  void SetValue(int index, double value);

  ///@{
  /**
   * Set/Get the component to threshold. Set this to a value greater than the number of
   * components in the selected data array to threshold by magnitude.
   */
  vtkSetMacro(InputArrayComponent, int);
  vtkGetMacro(InputArrayComponent, int);
  ///@}

protected:
  vtkSelectPointsByScalars() = default;
  ~vtkSelectPointsByScalars() = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkSelectPointsByScalars(const vtkSelectPointsByScalars&);
  void operator=(const vtkSelectPointsByScalars&);

  int InputArrayComponent;
  std::vector<int> ValuesToProcess;
};

#endif // vtkSelectPointsByScalars_h
