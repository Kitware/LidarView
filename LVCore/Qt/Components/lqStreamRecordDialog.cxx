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

#include "lqStreamRecorderInterface.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqFileChooserWidget.h>
#include <pqServerManagerModel.h>
#include <pqSettings.h>

#include <vtkSMProxy.h>

#include <QDir>
#include <QPushButton>
#include <QWidget>

constexpr const char* SETTING_NAME = "RecordFilename";

namespace
{
pqPipelineSource* retrivePipelineSource(QString name)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  for (pqPipelineSource* src : smmodel->findItems<pqPipelineSource*>())
  {
    if (name == QString(src->getProxy()->GetLogName()))
    {
      return src;
    }
  }
  return nullptr;
}
}

//-----------------------------------------------------------------------------
class lqStreamRecordDialog::lqInternals : public Ui::lqStreamRecordDialog
{
public:
  lqInternals(QDialog* parent, QList<lqStreamRecorderInterface*>& list)
    : recorderList(list)
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
      okButton->setEnabled(dir.exists());

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
  QList<lqStreamRecorderInterface*>& recorderList;
};

//-----------------------------------------------------------------------------
lqStreamRecordDialog::lqStreamRecordDialog(QWidget* parent,
  QList<lqStreamRecorderInterface*>& recorderList)
  : Superclass(parent)
  , Internals(new lqStreamRecordDialog::lqInternals(this, recorderList))
{
  auto& internals = *this->Internals;

  for (lqStreamRecorderInterface* recorder : recorderList)
  {
    internals.recorderComboBox->addItem(recorder->label());
  }
  if (recorderList.size() == 1)
  {
    internals.recorderLabel->setVisible(false);
    internals.recorderComboBox->setVisible(false);
    internals.recorderSeparatorLine->setVisible(false);
  }

  this->connect(internals.recorderComboBox,
    QOverload<int>::of(&QComboBox::currentIndexChanged),
    this,
    &lqStreamRecordDialog::onRecorderChanged);
  this->connect(internals.streamSelectList,
    &QListWidget::itemChanged,
    this,
    &lqStreamRecordDialog::updateDialogValidity);

  this->onRecorderChanged();
}

//-----------------------------------------------------------------------------
lqStreamRecordDialog::~lqStreamRecordDialog() = default;

//-----------------------------------------------------------------------------
void lqStreamRecordDialog::onRecorderChanged()
{
  auto& internals = *this->Internals;
  lqStreamRecorderInterface* recorder = this->getSelectedInterface();

  bool multipleStream = recorder->supportMultipleSources();
  internals.streamSelectList->setVisible(multipleStream);
  internals.streamSelectComboBox->setVisible(!multipleStream);
  internals.streamSelectList->clear();
  internals.streamSelectComboBox->clear();

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  for (pqPipelineSource* src : smmodel->findItems<pqPipelineSource*>())
  {
    if (recorder->canRecord(src))
    {
      if (multipleStream)
      {
        QListWidgetItem* item =
          new QListWidgetItem(src->getProxy()->GetLogName(), internals.streamSelectList);
        item->setCheckState(Qt::Checked);
      }
      else
      {
        internals.streamSelectComboBox->addItem(src->getProxy()->GetLogName());
      }
    }
  }

  this->updateDialogValidity();
}

//-----------------------------------------------------------------------------
void lqStreamRecordDialog::updateDialogValidity()
{
  auto& internals = *this->Internals;
  bool isEmpty = this->getSelectedInterface()->supportMultipleSources();

  for (int i = 0; i < internals.streamSelectList->count(); ++i)
  {
    QListWidgetItem* item = internals.streamSelectList->item(i);
    if (item->checkState() == Qt::Checked)
    {
      isEmpty = false;
    }
  }
  QPushButton* okButton = internals.buttonBox->button(QDialogButtonBox::Ok);
  okButton->setEnabled(!isEmpty);
  okButton->setToolTip(isEmpty ? "At least on stream must be selected!" : "");
}

//-----------------------------------------------------------------------------
lqStreamRecorderInterface* lqStreamRecordDialog::getSelectedInterface()
{
  auto& internals = *this->Internals;
  return internals.recorderList.at(internals.recorderComboBox->currentIndex());
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> lqStreamRecordDialog::getSelectedSources()
{
  auto& internals = *this->Internals;
  QList<pqPipelineSource*> selectedSource;

  if (this->getSelectedInterface()->supportMultipleSources())
  {
    for (int i = 0; i < internals.streamSelectList->count(); ++i)
    {
      QListWidgetItem* item = internals.streamSelectList->item(i);
      if (item->checkState() == Qt::Checked)
      {
        if (pqPipelineSource* src = ::retrivePipelineSource(item->text()))
        {
          selectedSource.append(src);
        }
      }
    }
  }
  else
  {
    if (auto* src = ::retrivePipelineSource(internals.streamSelectComboBox->currentText()))
    {
      selectedSource.append(src);
    }
  }
  return selectedSource;
}

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
