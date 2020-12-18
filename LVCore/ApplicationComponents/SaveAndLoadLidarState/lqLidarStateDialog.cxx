#include "lqLidarStateDialog.h"

#include <QLabel>
#include <QPushButton>

#include <string>

//-----------------------------------------------------------------------------
lqLidarStateDialog::lqLidarStateDialog(QWidget *parent,
                                       std::vector<propertyInfo>& propertiesVector,
                                       const std::string& instruction) :
  QDialog(parent)
{
  this->properties = propertiesVector;
  this->instructions = QString(instruction.c_str());

  QVBoxLayout* vbox = new QVBoxLayout;

  this->CreateDialog(vbox);

  // Add a QPushbutton
  QPushButton * button = new QPushButton("OK");
  this->connect(button, SIGNAL(pressed()), this, SLOT(accept()));
  vbox->addWidget(button);
  this->setLayout(vbox);
}

//-----------------------------------------------------------------------------
void lqLidarStateDialog::CreateDialog(QVBoxLayout *vbox)
{
  // Display an information message if there is no property to display
  if(this->properties.empty())
  {
    QLabel * label =  new QLabel(QString("No property available"));
    vbox->addWidget(label, 0, Qt::AlignCenter);
    return;
  }

  // Display a message to give a tip to the user :
  if(!this->instructions.isEmpty())
  {
    QLabel * label =  new QLabel(this->instructions);
    label->setStyleSheet("font: italic;font-size: 12px ; color: grey");
    vbox->addWidget(label, 0, Qt::AlignLeft);
  }

  for(unsigned int i = 0; i < this->properties.size(); i++)
  {
    propertyInfo currentProperty = this->properties[i];

    std::string proxyName = currentProperty.proxyName;

    // The property is the name of a proxy : we display it as a title
    if(currentProperty.isProxy())
    {
      QLabel * label =  new QLabel(QString(proxyName.c_str()));
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
    }
  }
}
