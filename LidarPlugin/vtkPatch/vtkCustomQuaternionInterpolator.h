/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCustomQuaternionInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCustomQuaternionInterpolator - interpolate a quaternion
// .SECTION Description
// This class is used to interpolate a series of quaternions representing
// the rotations of a 3D object.  The interpolation may be linear in form
// (using spherical linear interpolation SLERP), or via spline interpolation
// (using SQUAD). In either case the interpolation is specialized to
// quaternions since the interpolation occurs on the surface of the unit
// quaternion sphere.
//
// To use this class, specify at least two pairs of (t,q[4]) with the
// AddQuaternion() method.  Next interpolate the tuples with the
// InterpolateQuaternion(t,q[4]) method, where "t" must be in the range of
// (t_min,t_max) parameter values specified by the AddQuaternion() method (t
// is clamped otherwise), and q[4] is filled in by the method.
//
// There are several important background references. Ken Shoemake described
// the practical application of quaternions for the interpolation of rotation
// (K. Shoemake, "Animating rotation with quaternion curves", Computer
// Graphics (Siggraph '85) 19(3):245--254, 1985). Another fine reference
// (available on-line) is E. B. Dam, M. Koch, and M. Lillholm, Technical
// Report DIKU-TR-98/5, Dept. of Computer Science, University of Copenhagen,
// Denmark.
//
// .SECTION Caveats
// Note that for two or less quaternions, Slerp (linear) interpolation is
// performed even if spline interpolation is requested. Also, the tangents to
// the first and last segments of spline interpolation are (arbitrarily)
// defined by repeating the first and last quaternions.
//
// There are several methods particular to quaternions (norms, products,
// etc.) implemented interior to this class. These may be moved to a separate
// quaternion class at some point.
//
// .SECTION See also
// vtkMasterMindQuaternion


#ifndef vtkCustomQuaternionInterpolator_h
#define vtkCustomQuaternionInterpolator_h

#include "vtkCommonMathModule.h" // For export macro
#include "vtkObject.h"

class vtkCustomQuaterniond;
class vtkCustomQuaternionList;

class vtkCustomQuaternionInterpolator : public vtkObject
{
public:
  vtkTypeMacro(vtkCustomQuaternionInterpolator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate the class.
  static vtkCustomQuaternionInterpolator* New();

  // Description:
  // Return the number of quaternions in the list of quaternions to be
  // interpolated.
  int GetNumberOfQuaternions();

  // Description:
  // Obtain some information about the interpolation range. The numbers
  // returned (corresponding to parameter t, usually thought of as time)
  // are undefined if the list of transforms is empty. This is a convenience
  // method for interpolation.
  double GetMinimumT();
  double GetMaximumT();

  // Description:
  // Reset the class so that it contains no data; i.e., the array of (t,q[4])
  // information is discarded.
  void Initialize();

  // Description:
  // Add another quaternion to the list of quaternions to be interpolated.
  // Note that using the same time t value more than once replaces the
  // previous quaternion at t. At least one quaternions must be added to
  // define an interpolation functios.
  void AddQuaternion(double t, const vtkCustomQuaterniond& q);
  void AddQuaternion(double t, double q[4]);

  // Description:
  // Delete the quaternion at a particular parameter t. If there is no
  // quaternion tuple defined at t, then the method does nothing.
  void RemoveQuaternion(double t);

  // Description:
  // Interpolate the list of quaternions and determine a new quaternion
  // (i.e., fill in the quaternion provided). If t is outside the range of
  // (min,max) values, then t is clamped to lie within the range.
  void InterpolateQuaternion(double t, vtkCustomQuaterniond& q);
  void InterpolateQuaternion(double t, double q[4]);

//BTX
  // Description:
  // Enums to control the type of interpolation to use.
  enum {INTERPOLATION_TYPE_LINEAR=0,
        INTERPOLATION_TYPE_SPLINE
  };
//ETX

  // Description:
  // Specify which type of function to use for interpolation. By default
  // (SetInterpolationFunctionToSpline()), cubic spline interpolation using a
  // modifed Kochanek basis is employed. Otherwise, if
  // SetInterpolationFunctionToLinear() is invoked, linear spherical interpolation
  // is used between each pair of quaternions.
  vtkSetClampMacro(InterpolationType,int,INTERPOLATION_TYPE_LINEAR,
                   INTERPOLATION_TYPE_SPLINE);
  vtkGetMacro(InterpolationType,int);
  void SetInterpolationTypeToLinear()
    {this->SetInterpolationType(INTERPOLATION_TYPE_LINEAR);}
  void SetInterpolationTypeToSpline()
    {this->SetInterpolationType(INTERPOLATION_TYPE_SPLINE);}

protected:
  vtkCustomQuaternionInterpolator();
  virtual ~vtkCustomQuaternionInterpolator();

  // Specify the type of interpolation to use
  int InterpolationType;

  // Internal variables for interpolation functions
  vtkCustomQuaternionList *QuaternionList; //used for linear quaternion interpolation

private:
  vtkCustomQuaternionInterpolator(const vtkCustomQuaternionInterpolator&);  // Not implemented.
  void operator=(const vtkCustomQuaternionInterpolator&);  // Not implemented.

};

#endif
