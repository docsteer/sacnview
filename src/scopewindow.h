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

#ifndef SCOPEWINDOW_H
#define SCOPEWINDOW_H

class ScopeChannel;
class QTableWidgetItem;

#include <QtGui>
#include <QWidget>
#include <QButtonGroup>
#include "streamingacn.h"
#include "consts.h"

namespace Ui {
class ScopeWindow;
}

class ScopeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ScopeWindow(int universe = MIN_SACN_UNIVERSE, QWidget *parent = 0);
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
    QHash<quint16, sACNManager::tListener> m_universes;

    enum {
        COL_UNIVERSE,
        COL_ADDRESS,
        COL_ENABLED,
        COL_COLOUR,
        COL_TRIGGER,
        COL_16BIT
    };
    int m_defaultUniverse;
};

#endif // SCOPEWINDOW_H
