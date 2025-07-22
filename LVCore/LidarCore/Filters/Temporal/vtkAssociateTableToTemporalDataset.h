/*=========================================================================

  Program:   LidarView
  Module:    vtkAssociateTableToTemporalDataset.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkAssociateTableToTemporalDataset_h
#define vtkAssociateTableToTemporalDataset_h

#include <vtkDataSetAlgorithm.h>
#include <vtkNew.h>

#include "lvFiltersTemporalModule.h"

#include <string>

/**
 * The vtkAssociateTableToTemporalDataset filter operates on two inputs:
 * - A temporal dataset.
 * - A vtkTable containing a time array column.
 *
 * This filter identifies the row in the vtkTable that corresponds to the latest time
 * within the range defined by the current timestep and the next timestep (n+1).
 * It then appends all the data from this row to the field data of the input dataset.
 */
class LVFILTERSTEMPORAL_EXPORT vtkAssociateTableToTemporalDataset : public vtkDataSetAlgorithm
{
public:
  static vtkAssociateTableToTemporalDataset* New();
  vtkTypeMacro(vtkAssociateTableToTemporalDataset, vtkDataSetAlgorithm);

  ///@{
  /**
   * Get/Set the name of the time array used by the algorithm.
   * This time array is required.
   */
  vtkGetMacro(TimeArrayName, std::string);
  vtkSetMacro(TimeArrayName, std::string);
  ///@}

  /**
   * Retrieves the name of the validity array.
   * This array is included in the output field data and indicates whether the association algorithm
   * succeeded.
   */
  static const char* VALIDITY_ARRAY_NAME();

protected:
  vtkAssociateTableToTemporalDataset();
  ~vtkAssociateTableToTemporalDataset();

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkAssociateTableToTemporalDataset(const vtkAssociateTableToTemporalDataset&) /*= delete*/;
  void operator=(const vtkAssociateTableToTemporalDataset&) /*= delete*/;

  std::string TimeArrayName;
};

#endif // vtkAssociateTableToTemporalDataset_h
