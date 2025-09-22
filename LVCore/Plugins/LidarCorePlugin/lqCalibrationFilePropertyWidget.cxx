/*=========================================================================

  Program:   LidarView
  Module:    lqCalibrationFilePropertyWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqCalibrationFilePropertyWidget.h"
#include "ui_lqCalibrationFileWidget.h"

#include <vtkPVXMLElement.h>

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqFileDialog.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqSettings.h>
#include <pqStringVectorPropertyWidget.h>

#include <QCheckBox>
#include <QDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace
{
constexpr const char* LIST_KEY = "CalibrationList";
constexpr const char* INDEX_KEY = "CalibrationIndex";
}

//-----------------------------------------------------------------------------
class lqCalibrationFilePropertyWidget::lqInternals
{
public:
  bool NoDefaultCalibration = false;
  QString SettingsGroup;
  Ui::CalibrationFileWidget UI;

  //-----------------------------------------------------------------------------
  lqInternals(const char* settingGroupName)
    : SettingsGroup(settingGroupName)
  {
  }

  //-----------------------------------------------------------------------------
  QListWidgetItem* createEntry(QString path, QListWidget* parent)
  {
    QFileInfo info(path);
    QListWidgetItem* wi = new QListWidgetItem(parent);
    wi->setText(info.fileName());
    wi->setToolTip(path);
    wi->setData(Qt::UserRole, path);
    return wi;
  }

  //-----------------------------------------------------------------------------
  bool setFileListWidgetItem(QString toSearch)
  {
    QListWidget* calibWidget = this->UI.CalibrationFileListWidget;
    QFileInfo info(toSearch);
    QList<QListWidgetItem*> itemList = calibWidget->findItems(info.fileName(), Qt::MatchExactly);
    bool found = false;
    Q_FOREACH (QListWidgetItem* item, itemList)
    {
      if (item->data(Qt::UserRole).toString() == toSearch)
      {
        calibWidget->setCurrentItem(item);
        found = true;
      }
    }
    return found;
  }

  //-----------------------------------------------------------------------------
  void saveCalibrationList()
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QListWidget* listWidget = this->UI.CalibrationFileListWidget;
    if (listWidget->count() == 0)
    {
      return;
    }
    QStringList list;
    for (int i = 0; i < listWidget->count(); ++i)
    {
      list << listWidget->item(i)->data(Qt::UserRole).toString();
    }
    settings->beginGroup(this->SettingsGroup);
    settings->setValue(::LIST_KEY, list);
    settings->setValue(::INDEX_KEY, listWidget->currentRow());
    settings->endGroup();
  }

  //-----------------------------------------------------------------------------
  void loadCalibrationList()
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QListWidget* listWidget = this->UI.CalibrationFileListWidget;
    settings->beginGroup(this->SettingsGroup);
    QStringList list = settings->value(::LIST_KEY).toStringList();
    int index = settings->value(::INDEX_KEY, 0).toInt();
    settings->endGroup();
    for (const QString& fullname : list)
    {
      listWidget->addItem(this->createEntry(fullname, listWidget));
    }
    if (index < listWidget->count())
    {
      listWidget->setCurrentRow(index);
    }
  }

  //-----------------------------------------------------------------------------
  void resetCalibrationList()
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    settings->beginGroup(this->SettingsGroup);
    settings->remove(::LIST_KEY);
    settings->remove(::INDEX_KEY);
    settings->endGroup();
  }

  //-----------------------------------------------------------------------------
  QString chooseFile(lqCalibrationFilePropertyWidget* self,
    vtkSMProxy* proxy,
    vtkSMProperty* property) const
  {
    auto session = proxy->GetSession();
    auto hints = property->GetHints();

    // process hints.
    bool directoryMode, anyFile, browseLocalFileSystem;
    QString filter;
    pqStringVectorPropertyWidget::processFileChooserHints(
      hints, directoryMode, anyFile, filter, browseLocalFileSystem);

    pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
    auto server = browseLocalFileSystem ? nullptr : smModel->findServer(session);

    pqFileDialog dialog(server,
      self,
      tr("Select %1").arg(QCoreApplication::translate("ServerManagerXML", property->GetXMLLabel())),
      QString(),
      filter,
      false);
    if (directoryMode)
    {
      dialog.setFileMode(pqFileDialog::Directory);
    }
    else if (anyFile)
    {
      dialog.setFileMode(pqFileDialog::AnyFile);
    }
    else
    {
      // note: we select 1 file at a time here.
      dialog.setFileMode(pqFileDialog::ExistingFile);
    }

    if (dialog.exec() == QDialog::Accepted)
    {
      auto filesNames = dialog.getSelectedFiles();
      return filesNames.isEmpty() ? QString() : filesNames[0];
    }

    return QString();
  }
};

//-----------------------------------------------------------------------------
lqCalibrationFilePropertyWidget::lqCalibrationFilePropertyWidget(vtkSMProxy* smproxy,
  vtkSMProperty* smproperty,
  QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new lqCalibrationFilePropertyWidget::lqInternals(smproxy->GetXMLName()))
{
  this->setShowLabel(true);

  auto& internals = (*this->Internals);
  internals.UI.setupUi(this);
  internals.loadCalibrationList();

  QObject::connect(internals.UI.CalibrationFileListWidget,
    &QListWidget::itemSelectionChanged,
    this,
    &lqCalibrationFilePropertyWidget::filenameChanged);

  vtkPVXMLElement* hints = smproperty->GetHints();
  internals.NoDefaultCalibration = hints && hints->FindNestedElementByName("NoDefaultCalibration");
  if (internals.NoDefaultCalibration)
  {
    internals.UI.UseCustomCalibrationFile->setVisible(false);
  }
  else
  {
    internals.UI.GroupBox->setVisible(false);
    QObject::connect(internals.UI.UseCustomCalibrationFile,
      &QCheckBox::stateChanged,
      [this](int state)
      {
        bool isChecked = state == Qt::Checked;
        this->Internals->UI.GroupBox->setVisible(isChecked);
        Q_EMIT this->filenameChanged();
      });
  }

  auto addButtonReaction = [&internals, this, smproxy, smproperty]()
  {
    QString filename = internals.chooseFile(this, smproxy, smproperty);
    if (!filename.isEmpty())
    {
      QListWidget* calibWidget = internals.UI.CalibrationFileListWidget;
      auto entry = internals.createEntry(filename, calibWidget);
      calibWidget->addItem(entry);
      calibWidget->setCurrentRow(calibWidget->count() - 1);
    }
  };
  QObject::connect(internals.UI.AddButton, &QToolButton::clicked, addButtonReaction);

  auto removeButtonReaction = [&internals, smproxy]()
  {
    const int row = internals.UI.CalibrationFileListWidget->currentRow();
    delete internals.UI.CalibrationFileListWidget->takeItem(row);
  };
  QObject::connect(internals.UI.RemoveButton, &QToolButton::clicked, removeButtonReaction);

  this->setChangeAvailableAsChangeFinished(true);
  this->addPropertyLink(this, "filenameProp", SIGNAL(filenameChanged()), smproperty);
}

//-----------------------------------------------------------------------------
lqCalibrationFilePropertyWidget::~lqCalibrationFilePropertyWidget() = default;

//-----------------------------------------------------------------------------
QString lqCalibrationFilePropertyWidget::currentFilename() const
{
  auto& internals = (*this->Internals);
  QString filename;
  QListWidget* calibWidget = internals.UI.CalibrationFileListWidget;
  bool useCustomCalib = internals.UI.UseCustomCalibrationFile->isChecked() || internals.NoDefaultCalibration;
  if (calibWidget->count() != 0 && useCustomCalib)
  {
    const int row = calibWidget->currentRow();
    if (0 <= row && row < calibWidget->count())
    {
      filename = calibWidget->item(row)->data(Qt::UserRole).toString();
    }
  }
  internals.resetCalibrationList();
  internals.saveCalibrationList();
  return filename;
}

//-----------------------------------------------------------------------------
void lqCalibrationFilePropertyWidget::setCurrentFilename(QString filename)
{
  auto& internals = (*this->Internals);

  bool found = internals.setFileListWidgetItem(filename);
  QListWidget* calibWidget = internals.UI.CalibrationFileListWidget;
  if (!found && !filename.isEmpty())
  {
    auto entry = internals.createEntry(filename, calibWidget);
    calibWidget->addItem(entry);
    calibWidget->setCurrentRow(calibWidget->count() - 1);
  }
  internals.UI.UseCustomCalibrationFile->setChecked(!filename.isEmpty());
}
