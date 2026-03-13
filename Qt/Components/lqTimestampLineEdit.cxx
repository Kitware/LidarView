/*=========================================================================

  Program: LidarView
  Module:  lqTimestampLineEdit.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqTimestampLineEdit.h"

#include <QDoubleValidator>
#include <QFocusEvent>

#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

//-----------------------------------------------------------------------------
lqTimestampLineEdit::lqTimestampLineEdit(QWidget* _parent)
  : Superclass(_parent)
{
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->setValidator(validator);
  this->connect(this, &QLineEdit::editingFinished, this, &lqTimestampLineEdit::validateChanges);
}

//-----------------------------------------------------------------------------
lqTimestampLineEdit::~lqTimestampLineEdit() = default;

//-----------------------------------------------------------------------------
void lqTimestampLineEdit::setValidatorRange(double minimum, double maximum)
{
  this->ValidatorRange[0] = minimum;
  this->ValidatorRange[1] = maximum;
}

//-----------------------------------------------------------------------------
void lqTimestampLineEdit::setTimestamp(double timestamp)
{
  this->timestampValue = timestamp;
  this->updateUI();
}

//-----------------------------------------------------------------------------
void lqTimestampLineEdit::updateUI()
{
  if (this->IsFocused)
  {
    this->setText(QString::number(this->timestampValue, 'f', 3));
  }
  else
  {
    this->setText(lqTimestampLineEdit::convertTimestampToDate(this->timestampValue));
  }
  this->setToolTip(
    "UTC Time: " + lqTimestampLineEdit::convertTimestampToDate(this->timestampValue, true));
}

//-----------------------------------------------------------------------------
void lqTimestampLineEdit::validateChanges()
{
  double timestamp =
    std::clamp(this->text().toDouble(), this->ValidatorRange[0], this->ValidatorRange[1]);

  // Ignore minor timestamp differences (<= 1ms) due to line edit truncation.
  if (std::abs(this->timestampValue - timestamp) <= 1e-3)
  {
    return;
  }

  this->setTimestamp(timestamp);
  Q_EMIT this->timestampUpdated(this->timestampValue);
}

//-----------------------------------------------------------------------------
void lqTimestampLineEdit::focusInEvent(QFocusEvent* evt)
{
  this->Superclass::focusInEvent(evt);
  if (evt)
  {
    this->IsFocused = true;
    this->updateUI();
  }
}

//-----------------------------------------------------------------------------
void lqTimestampLineEdit::focusOutEvent(QFocusEvent* evt)
{
  this->Superclass::focusOutEvent(evt);
  if (evt)
  {
    this->IsFocused = false;
    this->updateUI();
  }
}

//-----------------------------------------------------------------------------
QString lqTimestampLineEdit::convertTimestampToDate(double time, bool fullDate)
{
  boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
  boost::posix_time::time_duration duration = boost::posix_time::seconds(static_cast<long>(time)) +
    boost::posix_time::microseconds(static_cast<long>((time - static_cast<long>(time)) * 1e6));
  boost::posix_time::ptime utcTimestamp = epoch + duration;

  boost::posix_time::ptime validTime(boost::gregorian::date(2000, 1, 1));
  if (fullDate && utcTimestamp > validTime)
  {
    return boost::posix_time::to_simple_string(utcTimestamp).c_str();
  }
  boost::posix_time::time_duration timeOfDay = utcTimestamp.time_of_day();
  std::ostringstream oss;
  oss << std::setw(2) << std::setfill('0') << timeOfDay.hours() << ":" << std::setw(2)
      << std::setfill('0') << timeOfDay.minutes() << ":" << std::setw(2) << std::setfill('0')
      << timeOfDay.seconds() << "." << std::setw(6) << std::setfill('0')
      << timeOfDay.fractional_seconds();
  return oss.str().c_str();
}
