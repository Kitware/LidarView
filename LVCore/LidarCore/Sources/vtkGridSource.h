/*=========================================================================

  Program: LidarView
  Module:  vtkGridSource.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class vtkGridSource
 *
 * Generates a vtkPolyData measurement grid plane
 */

#ifndef vtkGridSource_h
#define vtkGridSource_h

#include "lvSourcesModule.h"

#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

class LVSOURCES_EXPORT vtkGridSource : public vtkPolyDataAlgorithm
{
public:
  static vtkGridSource* New();
  vtkTypeMacro(vtkGridSource, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(GridNbTicks, int);
  vtkGetMacro(GridNbTicks, int);

  vtkSetMacro(Scale, double);
  vtkGetMacro(Scale, double);

  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);

  vtkSetVector3Macro(Normal, double);
  vtkGetVector3Macro(Normal, double);

  static vtkSmartPointer<vtkPolyData> CreateGrid(int gridNbTicks,
    double scale,
    double origin[3],
    double normal[3]);

protected:
  vtkGridSource();
  ~vtkGridSource();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int GridNbTicks;
  double Scale;
  double Origin[3];
  double Normal[3];

private:
  vtkGridSource(const vtkGridSource&);
  void operator=(const vtkGridSource&);
};

#endif
