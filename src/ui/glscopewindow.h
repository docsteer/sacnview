// Copyright 2023 Electronic Theatre Controls, Inc. or its affiliates
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

#pragma once

#include "consts.h"

#include <QWidget>
#include <QColorDialog>
#include <QStyledItemDelegate>

class GlScopeWidget;
class SteppedSpinBox;

class QCheckBox;
class QComboBox;
class QPushButton;
class QScrollBar;
class QSpinBox;
class QSplitter;
class QTableView;

class GlScopeWindow : public QWidget
{
  Q_OBJECT
public:
  explicit GlScopeWindow(int universe, QWidget* parent = nullptr);
  ~GlScopeWindow();

private:
  Q_SLOT void onRunningChanged(bool running);
  Q_SLOT void onTimeSliderMoved(int value);
  Q_SLOT void onTimeDivisionsChanged(int value);
  Q_SLOT void setTimeFormat(int value);

  Q_SLOT void setRecordMode(int idx);
  Q_SLOT void setTraceStyle(int idx);
  Q_SLOT void setVerticalScaleMode(int idx);
  Q_SLOT void onVerticalScaleChanged(int value);
  Q_SLOT void setTriggerType(int idx);

  Q_SLOT void addTrace(bool);
  Q_SLOT void removeTrace(bool);
  Q_SLOT void removeAllTraces(bool);

  Q_SLOT void saveTraces(bool);
  Q_SLOT void loadTraces(bool);

  Q_SLOT void onTriggered();
  Q_SLOT void onChkSyncViewsToggled(bool checked);

  // Signals to start/stop other open rx views
  Q_SIGNAL void startOtherViews();
  Q_SIGNAL void stopOtherViews();

private:
  QSplitter* m_splitter = nullptr;
  GlScopeWidget* m_scope = nullptr;
  QScrollBar* m_scrollTime = nullptr;
  QComboBox* m_recordMode = nullptr;
  QComboBox* m_traceStyle = nullptr;
  QSpinBox* m_spinRunTime = nullptr;
  SteppedSpinBox* m_spinVertScale = nullptr;
  SteppedSpinBox* m_spinTimeScale = nullptr;
  QComboBox* m_timeFormat = nullptr;
  QComboBox* m_triggerType = nullptr;
  QSpinBox* m_spinTriggerLevel = nullptr;
  QPushButton* m_btnStart = nullptr;
  QPushButton* m_btnStop = nullptr;
  QCheckBox* m_chkSyncViews = nullptr;
  QTableView* m_tableView = nullptr;

  // Widgets to disable when running and enable when stopped
  std::vector<QWidget*> m_disableWhenRunning;
  // Widgets to disable when trigger is Free Run
  std::vector<QWidget*> m_triggerSetup;

  int m_defaultUniverse = MIN_SACN_UNIVERSE;

  int m_lastTraceHue = 0;
  int m_lastTraceSat = 255;

  void updateTimeScrollBars();
  void updateConfiguration();
  void refreshButtons();
};

class ColorDialog : public QColorDialog
{
  Q_OBJECT

public:
  ColorDialog(QWidget* parent = nullptr) : QColorDialog(parent) {}

protected:
  void showEvent(QShowEvent* ev) override;
};

class ColorPickerDelegate : public QStyledItemDelegate
{
  Q_OBJECT

public:
  ColorPickerDelegate(QWidget* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& idx) const override;
  void destroyEditor(QWidget* editor, const QModelIndex& idx) const override;
  void setEditorData(QWidget* editor, const QModelIndex& idx) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& idx) const override;

private:
  QDialog* m_dialog = nullptr;
};
