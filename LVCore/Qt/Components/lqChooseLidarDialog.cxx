#include "lqChooseLidarDialog.h"

#include "ui_lqChooseLidarDialog.h"

#include <pqApplicationCore.h>
#include <pqSettings.h>

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>

//-----------------------------------------------------------------------------
class lqChooseLidarDialog::pqInternal : public Ui::lqChooseLidarDialog
{
};

//-----------------------------------------------------------------------------
lqChooseLidarDialog::lqChooseLidarDialog(QWidget* p, const QStringList& lidarNames)
  : QDialog(p)
  , Internal(new pqInternal)
{
  this->Internal->setupUi(this);

  Q_FOREACH (QString fullname, lidarNames)
  {
    QListWidgetItem* item = new QListWidgetItem();
    item->setText(fullname);
    item->setData(Qt::UserRole, "");
    this->Internal->ListLidar->addItem(item);
  }
}

//-----------------------------------------------------------------------------
int lqChooseLidarDialog::getSelectedLidarIndex() const
{
  return this->Internal->ListLidar->currentRow();
}
