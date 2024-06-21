/*=========================================================================

  Program: LidarView
  Module:  lqTimestampLineEdit.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqTimestampLineEdit_h
#define lqTimestampLineEdit_h

#include "lqComponentsModule.h"
#include <pqLineEdit.h>

#include <QDoubleValidator>
#include <QString>

/**
 * lqTimestampLineEdit displays its value in a UTC date format.
 * When the line edit is focused, it switches to show the
 * raw seconds value, allowing for direct editing.
 */
class LQCOMPONENTS_EXPORT lqTimestampLineEdit : public pqLineEdit
{
  Q_OBJECT
  typedef pqLineEdit Superclass;

public:
  lqTimestampLineEdit(QWidget* parent = nullptr);
  ~lqTimestampLineEdit() override;

  /**
   * Set minimum / maximum value the line editor can hold.
   */
  void setValidatorRange(double minimum, double maximum);

  /**
   * Convert the timestamp (in seconds) value to a "HH::MM::SS" string.
   * If fullData is on true, the returned string will also include the
   * full date in UTC format: "YYYY-MM-DD HH:MM:SS".
   */
  static QString convertTimestampToDate(double timestamp, bool fullDate = false);

public Q_SLOTS:
  /**
   * Set underlying timestamp value.
   */
  void setTimestamp(double timestamp);

Q_SIGNALS:
  /**
   * Emitted each time the timestamp is updated.
   */
  void timestampUpdated(double timestamp);

protected:
  ///@{
  /**
   * Methods that are called when the line edit is in/out focused.
   */
  void focusInEvent(QFocusEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  ///@}

private Q_SLOTS:
  void updateUI();
  void validateChanges();

private:
  Q_DISABLE_COPY(lqTimestampLineEdit)

  double timestampValue = 0.;
  bool IsFocused = false;
  double ValidatorRange[2] = { 0., 0. };
};

#endif
