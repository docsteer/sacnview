// Copyright 2016 Tom Barthel-Steer
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

#ifndef SCOPEWINDOW_H
#define SCOPEWINDOW_H

#define COL_UNIVERSE    0
#define COL_ADDRESS     1
#define COL_ENABLED     2
#define COL_COLOUR      3
#define COL_TRIGGER     4
#define COL_16BIT       5


class ScopeChannel;
class QTableWidgetItem;

#include <QtGui>
#include <QWidget>
#include <QButtonGroup>

namespace Ui {
class ScopeWindow;
}

class ScopeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ScopeWindow(QWidget *parent = 0);
    ~ScopeWindow();
private slots:
    void timebaseChanged(int value);
    void on_btnStart_pressed();
    void on_btnStop_pressed();
    void on_btnAddChannel_pressed();
    void on_btnRemoveChannel_pressed();
    void on_tableWidget_cellDoubleClicked(int row, int col);
    void on_tableWidget_itemChanged(QTableWidgetItem * item);
    void on_buttonGroup_buttonPressed(int id);
    void on_cbTriggerMode_currentIndexChanged(int index);
    void on_sbTriggerLevel_valueChanged(int value);
    void on_scopeWidget_stopped();
private:
    Ui::ScopeWindow *ui;
    QList<ScopeChannel *> m_channels;
    QButtonGroup *m_radioGroup;
};

#endif // SCOPEWINDOW_H
