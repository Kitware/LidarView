#ifndef LQLIDARSTATEDIALOG_H
#define LQLIDARSTATEDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QCheckBox>

#include <cstring>
#include <vector>

class propertyInfo
{
public:
  propertyInfo(std::string _proxyName, std::string _propName):
               proxyName(_proxyName), propertyName(_propName)
               {initCheckbox();}

  propertyInfo(std::string _proxyName, std::string _propName, std::vector<std::string>& _values):
               proxyName(_proxyName), propertyName(_propName), values(_values){initCheckbox();}

  std::string proxyName;
  std::string propertyName;
  std::vector<std::string> values;
  QCheckBox * checkbox;

  void initCheckbox()
  {
    checkbox = new QCheckBox(QString(propertyName.c_str()));
    checkbox ->setChecked(false);
    checkbox ->setCheckable(true);
  }

  std::string getCheckboxLabel()
  {
    return checkbox->text().toStdString();
  }

  bool isProxy()
  {
    return (proxyName.compare(this->propertyName) == 0);
  }

  std::string getValuesAsSingleString()
  {
    std::string concatenateAllValues = "";
    for(unsigned int i = 0; i < this->values.size(); i++)
    {
      if(i != 0)
      {
        concatenateAllValues = concatenateAllValues + " ";
      }
      concatenateAllValues = concatenateAllValues + values[i];
    }
    return concatenateAllValues;
  }
};

/**
 * @brief Dialog so the user can select the properties of a proxy to save
 */
class lqLidarStateDialog : public QDialog
{
  Q_OBJECT

public:
  explicit lqLidarStateDialog(QWidget *parent,
                              std::vector<propertyInfo>& propertiesVector,
                              const std::string& instruction = "");

  ~lqLidarStateDialog(){}

  void CreateDialog(QVBoxLayout * vbox);

  std::vector<propertyInfo> properties;

  QString instructions;
};
#endif // LQLIDARSTATEDIALOG_H


