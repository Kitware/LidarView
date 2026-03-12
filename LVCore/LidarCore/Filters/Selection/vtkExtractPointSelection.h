/*=========================================================================

  Program:   LidarView
  Module:    vtkExtractPointSelection.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkExtractPointSelection_h
#define vtkExtractPointSelection_h

// vtk includes
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include "lvFiltersSelectionModule.h"

/**
 * @class vtkExtractPointSelection
 * @brief extracts the selected points and convert them to a polydata
 */
class LVFILTERSSELECTION_EXPORT vtkExtractPointSelection : public vtkPolyDataAlgorithm
{
public:
  static vtkExtractPointSelection* New();
  vtkTypeMacro(vtkExtractPointSelection, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Convenience method to specify the selection connection (2nd input
   * port)
   */
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  vtkGetMacro(InvertSelection, bool);
  vtkSetMacro(InvertSelection, bool);

protected:
  // constructor / destructor
  vtkExtractPointSelection();
  ~vtkExtractPointSelection() = default;

  // Request data
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Invert the selection, equivalent to removing the selection
  bool InvertSelection = false;

private:
  // copy operators
  vtkExtractPointSelection(const vtkExtractPointSelection&);
  void operator=(const vtkExtractPointSelection&);
};

#endif // vtkExtractPointSelection_h
