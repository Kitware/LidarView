/*=========================================================================

  Program:   LidarView
  Module:    lqConditionalDefaultPropertyWidgetDecorator.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqConditionalDefaultPropertyWidgetDecorator_h
#define lqConditionalDefaultPropertyWidgetDecorator_h

#include "pqPropertyWidgetDecorator.h"
#include "vtkWeakPointer.h"

#include <string>
#include <unordered_map>

class vtkObject;

/**
 * lqConditionalDefaultPropertyWidgetDecorator is a widget decorator that
 * can change default value based on an update of an other property.
 * E.g LidarPort is changed based on LidarModel value.
 *
 * Note: Currently it is only supporting int, double and string vector property
 * of size 1.
 */
class lqConditionalDefaultPropertyWidgetDecorator : public pqPropertyWidgetDecorator
{
  Q_OBJECT
  typedef pqPropertyWidgetDecorator Superclass;

public:
  lqConditionalDefaultPropertyWidgetDecorator(vtkPVXMLElement* config,
    pqPropertyWidget* parentObject);
  virtual ~lqConditionalDefaultPropertyWidgetDecorator();

private Q_SLOTS:
  void connectedPropertyChanged();

private:
  Q_DISABLE_COPY(lqConditionalDefaultPropertyWidgetDecorator)

  vtkWeakPointer<vtkObject> ObservedObject;
  std::unordered_map<std::string, std::string> MappedDefaultValues;
  unsigned long ObserverId;
};

#endif
