#include "lqLidarStateDialog.h"

#include <QLabel>
#include <QPushButton>

#include <string>

//-----------------------------------------------------------------------------
lqLidarStateDialog::lqLidarStateDialog(QWidget* parent,
  std::vector<propertyInfo>& propertiesVector,
  const std::string& instruction)
  : QDialog(parent)
{
  this->properties = propertiesVector;
  this->instructions = QString(instruction.c_str());

  this->allCheckbox = new QCheckBox(QString("Select all"));
  connect(this->allCheckbox, SIGNAL(stateChanged(int)), this, SLOT(AllCheckboxStateUpdate(int)));

  QVBoxLayout* vbox = new QVBoxLayout;

  this->CreateStateDialog(vbox);

  // Add a QPushbutton
  QPushButton* button = new QPushButton("OK");
  this->connect(button, SIGNAL(pressed()), this, SLOT(accept()));
  vbox->addWidget(button);
  this->setLayout(vbox);
}

//-----------------------------------------------------------------------------
void lqLidarStateDialog::CreateStateDialog(QVBoxLayout* vbox)
{
  // Display an information message if there is no property to display
  if (this->properties.empty())
  {
    QLabel* label = new QLabel(QString("No property available"));
    vbox->addWidget(label, 0, Qt::AlignCenter);
    return;
  }

  // Display a message to give a tip to the user :
  if (!this->instructions.isEmpty())
  {
    QHBoxLayout* hboxLayout = new QHBoxLayout();
    QLabel* label = new QLabel(this->instructions);
    label->setStyleSheet("font: italic;font-size: 12px ; color: grey");
    hboxLayout->addWidget(label, 0, Qt::AlignLeft);
    hboxLayout->addWidget(this->allCheckbox, 0, Qt::AlignRight);
    vbox->addLayout(hboxLayout);
  }

  for (unsigned int i = 0; i < this->properties.size(); i++)
  {
    propertyInfo currentProperty = this->properties[i];

    std::string proxyName = currentProperty.proxyName;

    // The property is the name of a proxy : we display it as a title
    if (currentProperty.isProxy())
    {
      QLabel* label = new QLabel(QString(proxyName.c_str()));
      label->setStyleSheet("font-weight: bold;font-size: 14px ; color: black");
      vbox->addWidget(label, 0, Qt::AlignCenter);
    }
    else
    {
      std::string value = currentProperty.getValuesAsSingleString();

      QHBoxLayout* hboxLayout = new QHBoxLayout();
      QLabel* label = new QLabel(value.c_str());
      hboxLayout->addWidget(currentProperty.checkbox, 0, Qt::AlignLeft);
      hboxLayout->addWidget(label, 0, Qt::AlignLeft);
      vbox->addLayout(hboxLayout);

      QObject::connect(
        currentProperty.checkbox, SIGNAL(toggled(bool)), this, SLOT(UpdateAllCheckStates()));
    }
  }
}

//-----------------------------------------------------------------------------
void lqLidarStateDialog::UpdateAllCheckStates()
{
  size_t checked = 0;
  for (auto& cb : this->properties)
  {
    checked += (cb.checkbox->isChecked() ? 1 : 0);
  }

  QSignalBlocker sblocker(this->allCheckbox);
  if (checked == this->properties.size())
  {
    allCheckbox->setCheckState(Qt::Checked);
    allCheckbox->setTristate(false);
  }
  else if (checked == 0)
  {
    allCheckbox->setCheckState(Qt::Unchecked);
    allCheckbox->setTristate(false);
  }
  else
  {
    allCheckbox->setCheckState(Qt::PartiallyChecked);
  }
}

//-----------------------------------------------------------------------------
void lqLidarStateDialog::AllCheckboxStateUpdate(int checkState)
{
  for (auto& cb : this->properties)
  {
    if (!cb.isProxy())
    {
      QCheckBox* currentCheckbox = cb.checkbox;
      QSignalBlocker sblocker(currentCheckbox);
      currentCheckbox->setChecked(checkState == Qt::Checked);
    }
  }

  this->allCheckbox->setTristate(false);
}