/*=========================================================================

  Program:   LidarView
  Module:    lqMultiColumnSelectorWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqMultiColumnSelectorWidget_h
#define lqMultiColumnSelectorWidget_h

#include <QByteArray>
#include <QSet>
#include <QString>
#include <QTableWidget>
#include <QVector>
#include <pqPropertyWidget.h>
#include <vector>
class vtkObject;
class vtkCallbackCommand;

/**
 * @class   lqMultiColumnSelectorWidget
 *
 * Property widget that maps a list of target keys (e.g.
 * acc_x, w_y, X, Ry(Pitch)) to available table columns. The widget builds a
 * 2-column table, supports lazy header discovery via
 * information-only properties, and stages values so they are applied
 * consistently with the rest of the panel.
 */
class lqMultiColumnSelectorWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  lqMultiColumnSelectorWidget(vtkSMProxy* smproxy,
    vtkSMProperty* smproperty,
    QWidget* parent = nullptr);

  ~lqMultiColumnSelectorWidget() override;

  /**
   * Commit staged mapping values to the proxy. Called by the panel during Apply.
   */
  void apply() override;
  /**
   * Reset UI from proxy values. Called by the panel during Reset.
   */
  void reset() override;

private:
  struct TargetConfig
  {
    QString Target;
    QByteArray Property;
    QByteArray ToggleProperty;
  };

  /**
   * (Re)build the 2-column table based on current toggles and available headers.
   * Pulls information-only properties from the server and merges with locally
   * parsed headers (when first Apply hasn't run yet).
   */
  void populateTable();
  /**
   * Retrieve mapping configuration for a given target.
   */
  const TargetConfig* configForTarget(const QString&) const;
  /**
   * Initialize mapping table using XML hints.
   */
  void initializeMappings(vtkSMProperty*);
  /**
   * Return whether associated toggle property is enabled (defaults to true).
   */
  bool isTargetEnabled(const TargetConfig&) const;
  /**
   * Compute and set a compact table height based on visible rows to keep the
   * panel tidy.
   */
  void adjustTableHeight();

  QTableWidget* Table = nullptr;
  vtkSMProxy* Proxy = nullptr;
  bool IsRefreshing = false;
  std::vector<std::pair<vtkObject*, unsigned long>> ToggleObservers;

  // Tracks which targets were explicitly cleared by the user (kept as <select>)
  QSet<QString> ClearedTargets;
  QVector<TargetConfig> Mappings;
};

#endif // lqMultiColumnSelectorWidget_h
