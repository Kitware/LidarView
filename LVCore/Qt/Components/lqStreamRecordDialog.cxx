/*=========================================================================

  Program: LidarView
  Module:  lqStreamRecordDialog.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqStreamRecordDialog.h"
#include "ui_lqStreamRecordDialog.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqFileChooserWidget.h>
#include <pqSettings.h>

#include <QDir>
#include <QPushButton>
#include <QWidget>

constexpr const char* SETTING_NAME = "RecordFilename";

//-----------------------------------------------------------------------------
class lqStreamRecordDialog::lqInternals : public Ui::lqStreamRecordDialog
{
public:
  lqInternals(QDialog* parent)
  {
    this->setupUi(parent);
    this->dirChooser = new pqFileChooserWidget(parent);
    this->dirChooser->setUseDirectoryMode(true);
    this->dirChooser->setForceSingleFile(true);
    this->dirChooser->setTitle("Choose where to save PCAP recording");
    this->dirChooser->setServer(pqActiveObjects::instance().activeServer());

    pqSettings* settings = pqApplicationCore::instance()->settings();
    QString lastFilename = settings->value(::SETTING_NAME, "").toString();
    QDir dir(lastFilename);
    if (dir.exists())
    {
      this->dirChooser->setSingleFilename(lastFilename);
    }

    this->directoryPathLayout->addWidget(this->dirChooser);

    this->recordTimeLapsSpinBox->setValue(5);

    QPushButton* okButton = this->buttonBox->button(QDialogButtonBox::Ok);
    if (okButton)
    {
      okButton->setEnabled(false);

      QObject::connect(this->dirChooser,
        &pqFileChooserWidget::filenameChanged,
        [this, okButton]()
        {
          QString filename = this->dirChooser->singleFilename();
          okButton->setEnabled(!filename.isEmpty());
        });

      QObject::connect(okButton,
        &QAbstractButton::clicked,
        [this]()
        {
          QString filename = this->dirChooser->singleFilename();
          pqSettings* settings = pqApplicationCore::instance()->settings();
          settings->setValue(::SETTING_NAME, filename);
        });
    }
  };

  pqFileChooserWidget* dirChooser;
};

//-----------------------------------------------------------------------------
lqStreamRecordDialog::lqStreamRecordDialog(QWidget* parent)
  : Superclass(parent)
  , Internals(new lqStreamRecordDialog::lqInternals(this))
{
}

//-----------------------------------------------------------------------------
lqStreamRecordDialog::~lqStreamRecordDialog() = default;

//-----------------------------------------------------------------------------
QString lqStreamRecordDialog::getFilepath()
{
  return this->Internals->dirChooser->singleFilename();
}

//-----------------------------------------------------------------------------
bool lqStreamRecordDialog::isSplitRecordingEnabled()
{
  return this->Internals->enablePeriodicFileSaveCheckBox->isChecked();
}

//-----------------------------------------------------------------------------
unsigned int lqStreamRecordDialog::getSplitInterval()
{
  return this->Internals->recordTimeLapsSpinBox->value();
}
