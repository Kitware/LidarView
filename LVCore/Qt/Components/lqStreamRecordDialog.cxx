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
#include <pqFileChooserWidget.h>

#include <QPushButton>
#include <QWidget>

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
