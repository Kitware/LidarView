/*=========================================================================

  Program:   LidarView
  Module:    lqConditionalDefaultPropertyWidgetDecorator.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqConditionalDefaultPropertyWidgetDecorator.h"

#include <pqCoreUtilities.h>
#include <pqPropertyWidget.h>
#include <vtkCommand.h>
#include <vtkPVXMLElement.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMUncheckedPropertyHelper.h>

//-----------------------------------------------------------------------------
lqConditionalDefaultPropertyWidgetDecorator::lqConditionalDefaultPropertyWidgetDecorator(
  vtkPVXMLElement* config,
  pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  if (!vtkSMIntVectorProperty::SafeDownCast(parentObject->property()) &&
    !vtkSMDoubleVectorProperty::SafeDownCast(parentObject->property()) &&
    !vtkSMStringVectorProperty::SafeDownCast(parentObject->property()))
  {
    qWarning("ConditionalDefaultValueDecorator: Is only supported by int, double or string vector "
             "properties.");
    return;
  }
  if (vtkSMVectorProperty::SafeDownCast(parentObject->property())->GetNumberOfElements() != 1)
  {

    qWarning("ConditionalDefaultValueDecorator: Only support properties with one element.");
    return;
  }

  vtkSMProxy* proxy = parentObject->proxy();

  const char* targetPropertyName = config->GetAttribute("property");
  if (!targetPropertyName)
  {
    qWarning("ConditionalDefaultValueDecorator: Could not locate property attribute.");
    return;
  }
  vtkSMProperty* prop = proxy ? proxy->GetProperty(targetPropertyName) : nullptr;
  if (!prop)
  {
    qWarning() << "Could not locate property named '" << QString(targetPropertyName)
               << "'.  lqConditionalDefaultPropertyWidgetDecorator will have no effect.";
    return;
  }

  for (unsigned int idx = 0; idx < config->GetNumberOfNestedElements(); ++idx)
  {
    vtkPVXMLElement* elem = config->GetNestedElement(idx);
    const char* defaultValue = elem->GetAttribute("default_value");
    const char* targetValue = elem->GetAttribute("target_value");
    if (!targetValue || !defaultValue)
    {
      qWarning(
        "ConditionalDefaultValueDecorator: Could not locate default_value and/or target_value.");
      continue;
    }
    this->MappedDefaultValues[targetValue] = defaultValue;
  }

  this->ObservedObject = prop;
  this->ObserverId = pqCoreUtilities::connect(
    prop, vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(connectedPropertyChanged()));
}

//-----------------------------------------------------------------------------
lqConditionalDefaultPropertyWidgetDecorator::~lqConditionalDefaultPropertyWidgetDecorator()
{
  if (this->ObservedObject && this->ObserverId)
  {
    this->ObservedObject->RemoveObserver(this->ObserverId);
  }
}

//-----------------------------------------------------------------------------
void lqConditionalDefaultPropertyWidgetDecorator::connectedPropertyChanged()
{
  pqPropertyWidget* parentObject = this->parentWidget();
  vtkSMProperty* targetProp = vtkSMProperty::SafeDownCast(this->ObservedObject);
  if (targetProp)
  {
    const char* value = vtkSMUncheckedPropertyHelper(targetProp).GetAsString();
    if (this->MappedDefaultValues.find(value) != this->MappedDefaultValues.end())
    {
      std::string defaultValue = this->MappedDefaultValues[value];
      if (auto prop = vtkSMIntVectorProperty::SafeDownCast(parentObject->property()))
      {
        auto finder = [&prop](auto value)
        { return prop->GetElement(0) == std::stod(value.second); };
        auto found = std::find_if(
          this->MappedDefaultValues.cbegin(), this->MappedDefaultValues.cend(), finder);
        if (found != this->MappedDefaultValues.cend())
        {
          prop->SetElement(0, std::stoi(defaultValue));
        }
      }
      else if (auto prop = vtkSMDoubleVectorProperty::SafeDownCast(parentObject->property()))
      {
        auto finder = [&prop](auto value)
        { return prop->GetElement(0) == std::stod(value.second); };
        auto found = std::find_if(
          this->MappedDefaultValues.cbegin(), this->MappedDefaultValues.cend(), finder);
        if (found != this->MappedDefaultValues.cend())
        {
          prop->SetElement(0, std::stod(defaultValue));
        }
      }
      else if (auto prop = vtkSMStringVectorProperty::SafeDownCast(parentObject->property()))
      {
        auto finder = [&prop](auto value) { return prop->GetElement(0) == value.second; };
        auto found = std::find_if(
          this->MappedDefaultValues.cbegin(), this->MappedDefaultValues.cend(), finder);
        if (found != this->MappedDefaultValues.cend())
        {
          prop->SetElement(0, defaultValue.c_str());
        }
      }
    }
  }
}
