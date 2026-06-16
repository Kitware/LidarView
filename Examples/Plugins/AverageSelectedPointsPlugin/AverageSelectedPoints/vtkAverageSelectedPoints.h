/*=========================================================================

  Program: LidarView
  Module:  vtkAverageSelectedPoints.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkAverageSelectedPoints_h
#define vtkAverageSelectedPoints_h

// vtk includes
#include <vtkPolyData.h>
#include <vtkSetGet.h>
#include <vtkSmartPointer.h>
#include <vtkTableAlgorithm.h>

class vtkImplicitFunction;

/**
 * vtkAverageSelectedPoints
 * Compute the average z height of selected points and display a text with the result.
 */
class vtkAverageSelectedPoints : public vtkTableAlgorithm
{
public:
  static vtkAverageSelectedPoints* New();
  vtkTypeMacro(vtkAverageSelectedPoints, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the MTime taking into account changes to the implicit function
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Specify the implicit function for inside/outside checks.
   */
  void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction, vtkImplicitFunction);
  ///@}

protected:
  // constructor / destructor
  vtkAverageSelectedPoints();
  ~vtkAverageSelectedPoints();

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  // copy operators
  vtkAverageSelectedPoints(const vtkAverageSelectedPoints&);
  void operator=(const vtkAverageSelectedPoints&);

  vtkImplicitFunction* ImplicitFunction = nullptr;
};

#endif // vtkAverageSelectedPoints_h
