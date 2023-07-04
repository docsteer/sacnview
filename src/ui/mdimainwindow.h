// Copyright 2016 Tom Steer
// http://www.tomsteer.net
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MDIMAINWINDOW_H
#define MDIMAINWINDOW_H

#include <QMainWindow>
#include "themes/themes.h"
#include "sacnuniverselistmodel.h"
#include "sacndiscoveredsourcelistmodel.h"
#include "sacnsynclistmodel.h"

#include "preferences.h"

class PreferencesDialog;

class QMdiArea;

namespace Ui {
  class MDIMainWindow;
}

class MDIMainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MDIMainWindow(QWidget* parent = 0);
  ~MDIMainWindow();

  /// Start all receiver views
  Q_SIGNAL void startReceiverViews();
  /// Stop all receiver views
  Q_SIGNAL void stopReceiverViews();

protected:
  void showEvent(QShowEvent* ev) override;

public:
  QWidget* showWidgetAsSubWindow(QWidget* w);
  void saveSubWindows();
  void restoreSubWindows();

protected slots:
  void on_actionScopeView_triggered(bool checked);
  void on_actionRecieve_triggered(bool checked);
  void on_actionMultiView_triggered(bool checked);
  void on_actionTranmsit_triggered(bool checked);
  void on_actionSettings_triggered(bool checked);
  void on_actionSnapshot_triggered(bool checked);
  void on_btnUnivListBack_pressed();
  void on_btnUnivListForward_pressed();
  void on_sbUniverseList_valueChanged(int value);

  void universeDoubleClick(const QModelIndex& index);
  void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);

private slots:
  void on_actionAbout_triggered(bool checked);

  void on_actionMultiUniverse_triggered();

  void on_actionPCAPPlayback_triggered();

  void on_pbFewer_clicked();

  void on_pbMore_clicked();

  void subWindowRemoved();

  void applyPrefs();
private:
  Ui::MDIMainWindow* ui = nullptr;
  sACNUniverseListModel* m_model = nullptr;
  sACNDiscoveredSourceListModel* m_modelDiscovered = nullptr;
  sACNDiscoveredSourceListProxy* m_proxyDiscovered = nullptr;
  sACNSyncListModel* m_modelSync = nullptr;
  sACNSyncListModel::proxy* m_proxySync = nullptr;

  // Dock/centralWidget
  QMdiArea* m_mdiArea = nullptr;
  // Floating windows
  QWidgetList m_subWindows;

  WindowMode m_currentWindowMode = WindowMode::COUNT;

  // Dialogs
  PreferencesDialog* m_prefDialog = nullptr;

  int getSelectedUniverse();
  QWidget* addMdiWidget(QWidget* w);
  QWidget* addFloatWidget(QWidget* w);

  static void StoreWidgetGeometry(const QWidget* window, const QWidget* widget, QList<SubWindowInfo>& result);
};

#endif // MDIMAINWINDOW_H
