#include "lqStreamRecordDialog.h"
#include "ui_lqStreamRecordDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

//-----------------------------------------------------------------------------
lqStreamRecordDialog::lqStreamRecordDialog(QWidget *parent, const QString& filename) :
  QDialog(parent),
  ui(new Ui::lqStreamRecordDialog)
{
  ui->setupUi(this);

  this->connect(ui->RecordingLineEdit, SIGNAL(textChanged(QString)),SLOT(onRecordingFileChange(QString)));
  this->connect(ui->PathToolButton, SIGNAL(clicked(bool)), SLOT(onModifyPath()));

  QFileInfo fileInfo(filename);
  this->ui->PathLineEdit->setText(fileInfo.absoluteDir().absolutePath());
  this->ui->RecordingLineEdit->setText(fileInfo.baseName()+".pcap");
}

//-----------------------------------------------------------------------------
lqStreamRecordDialog::~lqStreamRecordDialog()
{
  delete ui;
}

//-----------------------------------------------------------------------------
void lqStreamRecordDialog::onRecordingFileChange(const QString& str)
{
  QString file = str.endsWith(".pcap") ? str.chopped(5) : str;
  this->ui->CalibrationLineEdit->setText(file + ".json");
}

//-----------------------------------------------------------------------------
void lqStreamRecordDialog::onModifyPath()
{
  QString dir = QFileDialog::getExistingDirectory(this, tr("Path:"), QString());
  if (!dir.isEmpty())
  {
    this->ui->PathLineEdit->setText(dir);
  }
}

//-----------------------------------------------------------------------------
QString lqStreamRecordDialog::recordingFile()
{
  return this->ui->PathLineEdit->text() + '/' + this->ui->RecordingLineEdit->text();
}

//-----------------------------------------------------------------------------
QString lqStreamRecordDialog::calibrationFile()
{
  return this->ui->PathLineEdit->text() + '/' + this->ui->CalibrationLineEdit->text();
}
