#include "lqSaveLASDialog.h"
#include "ui_lqSaveLASDialog.h"

#include <QMessageBox>
#include <QDebug>

//-----------------------------------------------------------------------------
lqSaveLASDialog::lqSaveLASDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::lqSaveLASDialog)
{
  ui->setupUi(this);

  // Fill default values
  ui->ExportTypeComboBox->addItem("UTM", 0);
  ui->ExportTypeComboBox->addItem("Lat/Lon", 1);

  ui->UTMZoneSpinBox->setValue(31);
  ui->WriteColorCheckBox->setChecked(false);
  ui->WriteSRSCheckBox->setChecked(false);
}

//-----------------------------------------------------------------------------
bool lqSaveLASDialog::WriteSRS()
{
  return this->ui->WriteSRSCheckBox->isChecked();
}

//-----------------------------------------------------------------------------
bool lqSaveLASDialog::WriteColor()
{
  return this->ui->WriteColorCheckBox->isChecked();
}

//-----------------------------------------------------------------------------
int lqSaveLASDialog::UTMZone()
{
  return this->ui->UTMZoneSpinBox->value();
}

//-----------------------------------------------------------------------------
int lqSaveLASDialog::ExportType()
{
  return this->ui->ExportTypeComboBox->currentIndex();
}

//-----------------------------------------------------------------------------
lqSaveLASDialog::~lqSaveLASDialog()
{
  delete ui;
}



