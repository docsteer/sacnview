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

#include <QWidget>

class GlScopeWidget;

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
  explicit GlScopeWindow(QWidget* parent = nullptr);
  ~GlScopeWindow();

private:
  Q_SLOT void onRunningChanged(bool running);

  Q_SLOT void setVerticalScaleMode(int idx);
  Q_SLOT void setTriggerType(int idx);

  Q_SLOT void saveTraces(bool);
  Q_SLOT void loadTraces(bool);

private:
  QSplitter* m_splitter = nullptr;
  GlScopeWidget* m_scope = nullptr;
  QScrollBar* m_scrollTime = nullptr;
  QSpinBox* m_spinTimeScale = nullptr;
  QComboBox* m_triggerType = nullptr;
  QSpinBox* m_spinTriggerLevel = nullptr;
  QSpinBox* m_spinTriggerDelay = nullptr;
  QPushButton* m_btnStart = nullptr;
  QPushButton* m_btnStop = nullptr;
  QTableView* m_tableView = nullptr;

  void updateScrollBars();
};
