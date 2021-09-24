/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vvtkPCLRansacModel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPCLRansacModel_h
#define vtkPCLRansacModel_h

// vtk includes
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkSmartPointer.h>

#include "LidarCoreModule.h"

/**
 * @brief The vtkPCLRansacModel class will quickly be replace by classes from the pcl plugin
 * so no time should be spend developping this class
 */
class LIDARCORE_EXPORT vtkPCLRansacModel : public vtkPolyDataAlgorithm
{
  public:
  static vtkPCLRansacModel *New();
  vtkTypeMacro(vtkPCLRansacModel, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Model {
    Circle2D = 0,
    Circle3D,
    Cone,       // not implemented
    Cylinder,   // not implemented
    Sphere,
    Line,
    Plane
  };

  vtkGetMacro(DistanceThreshold, double)
  vtkSetMacro(DistanceThreshold, double)

  vtkGetMacro(Probability, double)
  vtkSetMacro(Probability, double)

  vtkGetMacro(MaxIterations, int)
  vtkSetMacro(MaxIterations, int)

  vtkGetMacro(ModelType, int)
  vtkSetMacro(ModelType, int)

protected:
  // constructor / destructor
  vtkPCLRansacModel();
  ~vtkPCLRansacModel();

  // Request data
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  //! Maximum distance from point to model, to consider the point part of the model
  double DistanceThreshold;

  //! Desired probability of choosing at least one sample free from outliers.
  double Probability;

  //! Maximum number of iterations.
  int MaxIterations;

  //! Model to approximate
  int ModelType;


private:
  // copy operators
  vtkPCLRansacModel(const vtkPCLRansacModel&);
  void operator=(const vtkPCLRansacModel&);
};

#endif // vtkPCLRansacModel_h
