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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "translations/translationdialog.h"
#include <QDialog>
#include <QNetworkInterface>

class QRadioButton;


namespace Ui {
  class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
  Q_OBJECT

public:
  explicit PreferencesDialog(QWidget* parent = 0);
  ~PreferencesDialog();

  Q_SIGNAL void storeWindowLayoutNow();

protected:
  void showEvent(QShowEvent* e) override;

private slots:
  void on_buttonBox_accepted();

private:
  Ui::PreferencesDialog* ui;
  QList<QNetworkInterface> m_interfaceList;
  TranslationDialog* m_translation;

};

#endif // PREFERENCESDIALOG_H
