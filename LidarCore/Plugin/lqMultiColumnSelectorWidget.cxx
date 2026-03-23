/*=========================================================================

  Program:   LidarView
  Module:    lqMultiColumnSelectorWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqMultiColumnSelectorWidget.h"

#include <QComboBox>
#include <QFile>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include <cstring>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkPVXMLElement.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMUncheckedPropertyHelper.h>
#include <vtkSmartPointer.h>

lqMultiColumnSelectorWidget::lqMultiColumnSelectorWidget(vtkSMProxy* smproxy,
  vtkSMProperty* smproperty,
  QWidget* parent)
  : Superclass(smproxy, parent)
  , Proxy(smproxy)
{
  this->initializeMappings(smproperty);
  // Build 2-column mapping table (Target - CSV Column)
  auto* layout = new QVBoxLayout(this);
  this->setShowLabel(false);

  this->Table = new QTableWidget(this);
  Table->setColumnCount(2);
  Table->setHorizontalHeaderLabels({ tr("Target"), tr("CSV Column") });
  Table->horizontalHeader()->setStretchLastSection(true);
  Table->verticalHeader()->setVisible(false);
  Table->setShowGrid(true);
  Table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  layout->addWidget(Table);

  this->setLayout(layout);

  this->setChangeAvailableAsChangeFinished(true);

  // Rebuild table when toggle properties change
  auto observeToggle = [this](const QString& name)
  {
    if (name.isEmpty())
    {
      return;
    }
    QByteArray cname = name.toUtf8();
    if (auto* p = this->Proxy->GetProperty(cname.constData()))
    {
      vtkSmartPointer<vtkCallbackCommand> cb = vtkSmartPointer<vtkCallbackCommand>::New();
      cb->SetClientData(this);
      cb->SetCallback(
        [](vtkObject*, unsigned long, void* cd, void*)
        {
          auto* self = static_cast<lqMultiColumnSelectorWidget*>(cd);
          if (!self->IsRefreshing)
            self->populateTable();
        });
      unsigned long oid = p->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, cb);
      this->ToggleObservers.emplace_back(p, oid);
    }
  };

  QSet<QString> toggles;
  for (const auto& cfg : this->Mappings)
  {
    if (!cfg.ToggleProperty.isEmpty())
    {
      toggles.insert(QString::fromUtf8(cfg.ToggleProperty));
    }
  }
  for (const auto& name : toggles)
  {
    observeToggle(name);
  }

  // Build the table
  this->populateTable();
}

//-----------------------------------------------------------------------------
lqMultiColumnSelectorWidget::~lqMultiColumnSelectorWidget()
{
  for (auto& pr : this->ToggleObservers)
  {
    if (pr.first && pr.second)
    {
      pr.first->RemoveObserver(pr.second);
    }
  }
}

//-----------------------------------------------------------------------------
const lqMultiColumnSelectorWidget::TargetConfig* lqMultiColumnSelectorWidget::configForTarget(
  const QString& t) const
{
  for (const auto& cfg : this->Mappings)
  {
    if (cfg.Target == t)
    {
      return &cfg;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void lqMultiColumnSelectorWidget::initializeMappings(vtkSMProperty* smproperty)
{
  this->Mappings.clear();

  if (smproperty)
  {
    if (vtkPVXMLElement* hints = smproperty->GetHints())
    {
      if (vtkPVXMLElement* mapping = hints->FindNestedElementByName("ColumnSelectorMapping"))
      {
        for (unsigned int i = 0, n = mapping->GetNumberOfNestedElements(); i < n; ++i)
        {
          vtkPVXMLElement* pairElem = mapping->GetNestedElement(i);
          if (!pairElem || std::strcmp(pairElem->GetName(), "ColumnPair") != 0)
          {
            continue;
          }
          const char* key = pairElem->GetAttribute("key");
          const char* prop = pairElem->GetAttribute("property_name");
          if (!(key && *key && prop && *prop))
          {
            continue;
          }

          TargetConfig cfg;
          cfg.Target = QString::fromUtf8(key);
          cfg.Property = QByteArray(prop);
          if (const char* toggle = pairElem->GetAttribute("toggle_property"))
          {
            if (*toggle)
            {
              cfg.ToggleProperty = QByteArray(toggle);
            }
          }
          this->Mappings.push_back(cfg);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool lqMultiColumnSelectorWidget::isTargetEnabled(const TargetConfig& cfg) const
{
  if (cfg.ToggleProperty.isEmpty())
  {
    return true;
  }
  if (auto* p = this->Proxy->GetProperty(cfg.ToggleProperty.constData()))
  {
    return vtkSMUncheckedPropertyHelper(p).GetAsInt() != 0;
  }
  return false;
}

//-----------------------------------------------------------------------------
void lqMultiColumnSelectorWidget::populateTable()
{
  this->IsRefreshing = true;

  QStringList columns;
  {
    if (auto* headerProp = this->Proxy->GetProperty("HeaderColumns"))
    {
      this->Proxy->UpdatePropertyInformation(headerProp);
    }

    // Fetch CSV headers from server-side HeaderColumns property
    vtkSMPropertyHelper header(this->Proxy, "HeaderColumns", true);
    header.UpdateValueFromServer();
    for (unsigned int j = 0, hn = header.GetNumberOfElements(); j < hn; ++j)
    {
      const char* col = header.GetAsString(j);
      if (col && *col)
      {
        columns << QString::fromUtf8(col);
      }
    }

    // If server header is empty, try parsing first CSV line locally
    if (columns.isEmpty())
    {
      const char* uncheckedFile = nullptr;
      if (auto* fp = this->Proxy->GetProperty("FileName"))
      {
        uncheckedFile = vtkSMUncheckedPropertyHelper(fp).GetAsString();
      }
      if (uncheckedFile && *uncheckedFile)
      {
        const QString path = QString::fromUtf8(uncheckedFile);
        QFile f(path);
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
          QTextStream ts(&f);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
          ts.setCodec("UTF-8");
#else
          ts.setEncoding(QStringConverter::Utf8);
#endif
          QString line = ts.readLine();

          QStringList toks;
          toks.reserve(32);
          bool inQuotes = false;
          QString cur;
          for (int i = 0; i < line.size(); ++i)
          {
            const QChar ch = line.at(i);
            if (ch == '"')
            {
              if (inQuotes && i + 1 < line.size() && line.at(i + 1) == '"')
              {
                cur += '"';
                ++i;
              }
              else
              {
                inQuotes = !inQuotes;
              }
            }
            else if (ch == ',' && !inQuotes)
            {
              toks << cur.trimmed();
              cur.clear();
            }
            else
            {
              cur += ch;
            }
          }
          toks << cur.trimmed();
          for (const auto& t : toks)
          {
            if (!t.isEmpty() && !columns.contains(t))
              columns << t;
          }
        }
      }
    }
  }

  // Build target list based on enabled modules
  QStringList targets;
  for (const auto& cfg : this->Mappings)
  {
    if (this->isTargetEnabled(cfg))
    {
      targets << cfg.Target;
    }
  }

  // Clear table rows; return early if no targets
  this->Table->setRowCount(0);
  if (targets.isEmpty())
  {
    this->IsRefreshing = false;
    return;
  }

  // Set table row count according to targets
  this->Table->setRowCount(targets.size());
  for (int i = 0; i < targets.size(); ++i)
  {
    const QString& target = targets[i];
    const TargetConfig* cfg = this->configForTarget(target);
    const char* propName = cfg ? cfg->Property.constData() : nullptr;

    // Column 0: show target name
    this->Table->setItem(i, 0, new QTableWidgetItem(target));

    // Column 1: combo box for selecting CSV column
    auto* combo = new QComboBox(this);
    {
      QSignalBlocker b(combo);
      combo->addItem(tr("<select>"));
      combo->addItems(columns);

      // preselected: tracks existing value; cleared: tracks explicit clears
      bool preselected = false;
      bool cleared = this->ClearedTargets.contains(target);
      if (propName)
      {
        if (auto* p = this->Proxy->GetProperty(propName))
        {
          if (const char* val = vtkSMPropertyHelper(p).GetAsString())
          {
            if (*val)
            {
              // If property is non-empty: select mapped column
              const QString mapped = QString::fromUtf8(val);
              int idx = combo->findText(mapped);
              if (idx < 0)
              {
                combo->addItem(mapped);
                idx = combo->findText(mapped);
              }
              if (idx >= 0)
              {
                combo->setCurrentIndex(idx);
                preselected = true;
              }
            }
            else
            {
              // Property value is empty. Consider it cleared ONLY if we
              // previously recorded an explicit clear for this target.
              // Otherwise treat as unset to allow default auto-matching.
              if (this->ClearedTargets.contains(target))
              {
                cleared = true;
              }
            }
          }
        }
        // Store target info in combo properties for signal callbacks
        combo->setProperty("lv_target_property",
          cfg ? QString::fromUtf8(cfg->Property) : QString::fromUtf8(propName));
        combo->setProperty("lv_target_name", target);
      }

      // If neither preselected nor cleared, try default auto-match
      if (!preselected && !cleared)
      {
        int idx = combo->findText(target, Qt::MatchFixedString);
        if (idx >= 0)
        {
          combo->setCurrentIndex(idx);
          if (propName)
          {
            // Persist default mapping into proxy property
            const QByteArray pname = QByteArray(propName);
            const QByteArray pval = target.toUtf8();
            vtkSMUncheckedPropertyHelper hU(this->Proxy, pname.constData());
            hU.Set(pval.constData());
            vtkSMPropertyHelper hC(this->Proxy, pname.constData());
            hC.Set(pval.constData());
            Q_EMIT this->changeAvailable();
          }
        }
        // Otherwise keep <select>, let reader fallback
      }
    }

    auto onComboChanged = [this](const QString& text)
    {
      if (this->IsRefreshing)
      {
        return;
      }

      auto* senderCombo = qobject_cast<QComboBox*>(QObject::sender());
      if (!senderCombo)
      {
        return;
      }

      QString prop = senderCombo->property("lv_target_property").toString();
      QString target = senderCombo->property("lv_target_name").toString();
      if (prop.isEmpty())
      {
        return;
      }

      // Update property: <select> means clear, otherwise set selected column
      const QByteArray pname = prop.toUtf8();
      const char* pval = nullptr;

      if (text.startsWith('<'))
      {
        pval = "";
        if (!target.isEmpty())
        {
          this->ClearedTargets.insert(target);
        }
      }
      else
      {
        pval = text.toUtf8().constData();
        if (!target.isEmpty())
        {
          this->ClearedTargets.remove(target);
        }
      }

      vtkSMUncheckedPropertyHelper hU(this->Proxy, pname.constData());
      hU.Set(pval);
      vtkSMPropertyHelper hC(this->Proxy, pname.constData());
      hC.Set(pval);
      Q_EMIT this->changeAvailable();
    };

    // Connect combo change event
    this->connect(combo, &QComboBox::currentTextChanged, this, onComboChanged);

    // Place combo box into table column 1
    this->Table->setCellWidget(i, 1, combo);
  }

  // End refresh: reset state and adjust table height
  this->IsRefreshing = false;
  this->adjustTableHeight();
}

//-----------------------------------------------------------------------------
void lqMultiColumnSelectorWidget::adjustTableHeight()
{
  const int rows = this->Table->rowCount();
  if (rows <= 0)
  {
    int h = this->Table->horizontalHeader()->height() + this->Table->frameWidth() * 2;
    this->Table->setMinimumHeight(h);
    this->Table->setMaximumHeight(h);
    return;
  }

  const int maxVisible = 6;
  const int visible = std::min(rows, maxVisible);
  int total = this->Table->horizontalHeader()->height() + this->Table->frameWidth() * 2;
  for (int r = 0; r < visible; ++r)
  {
    total += this->Table->rowHeight(r);
  }
  this->Table->setMinimumHeight(total);
  this->Table->setMaximumHeight(total);
}

//-----------------------------------------------------------------------------
void lqMultiColumnSelectorWidget::apply()
{
  // If Proxy or Table is missing, just call superclass apply and return
  if (!this->Proxy || !this->Table)
  {
    this->Superclass::apply();
    return;
  }

  // Get number of rows and iterate each target + combo box
  const int rows = this->Table->rowCount();
  for (int r = 0; r < rows; ++r)
  {
    // Column 0: target name, Column 1: combo box
    auto* item = this->Table->item(r, 0);
    auto* combo = qobject_cast<QComboBox*>(this->Table->cellWidget(r, 1));
    if (!item || !combo)
    {
      continue;
    }
    // Map target name to its proxy property name
    const QString target = item->text();
    const TargetConfig* cfg = this->configForTarget(target);
    if (!cfg)
    {
      continue;
    }
    const char* propName = cfg->Property.constData();

    // Get current selection text
    const QString sel = combo->currentText();
    vtkSMPropertyHelper h(this->Proxy, propName);
    if (sel.isEmpty() || sel.startsWith('<'))
    {
      h.Set("");
      this->ClearedTargets.insert(item->text());
    }
    else
    {
      h.Set(sel.toUtf8().constData());
      this->ClearedTargets.remove(item->text());
    }
  }
  // Call superclass apply to finish property commit and pipeline update
  this->Superclass::apply();
}

//-----------------------------------------------------------------------------
void lqMultiColumnSelectorWidget::reset()
{
  this->Superclass::reset();
  this->populateTable(); // Rebuild from proxy values
}
