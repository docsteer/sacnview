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

#ifndef MULTIUNIVERSE_H
#define MULTIUNIVERSE_H

#include <QWidget>
#include <QLabel>
#include <QSignalMapper>
#include "consts.h"
#include "streamingacn.h"
#include "sacneffectengine.h"
#include "consts.h"

namespace Ui {
class MultiUniverse;
}

class MultiUniverse : public QWidget
{
    Q_OBJECT

public:
    explicit MultiUniverse(int firstUniverse = MIN_SACN_UNIVERSE, QWidget *parent = 0);
    ~MultiUniverse();
private slots:
    void on_btnAddRow_pressed();
    void on_btnRemoveRow_pressed();
    void on_tableWidget_cellChanged(int row, int column);
    void on_btnAddMulti_clicked();

    void universeChanged(int value);
    void startChanged(int value);
    void endChanged(int value);
    void fxChanged(int value);
    void enableChanged(bool enable);
    void removeWidgetFromIndex(QObject *o);
    void controlSliderMoved(int value);
    void controlComboChanged(int value);
    void priorityChanged(int value);
    void senderTimedout(int index);
private:
    Ui::MultiUniverse *ui;
    CID m_cid;
    QList <sACNManager::tSender> m_senders;
    QList <sACNEffectEngine *> m_fxEngines;
    QHash<QWidget*, sACNEffectEngine *> m_widgetToFxEngine;
    QHash<QWidget*, sACNSentUniverse *> m_widgetToSender;
    QHash<QWidget*, QLabel *> m_widgetToLevelLabel;
    int m_firstUniverse;
    enum
    {
        COL_ENABLED,
        COL_NAME,
        COL_UNIVERSE,
        COL_STARTADDR,
        COL_ENDADDR,
        COL_EFFECT,
        COL_PRIORITY,
        COL_CONTROL
    };
    QSignalMapper *m_timeoutMapper;

    void setupControl(int row, sACNEffectEngine::FxMode mode, int value);
    void addSource(int universe, int min_address, int max_address, sACNEffectEngine::FxMode mode,
           QString name, bool startSending, int level = 0, int rate = 5, int priority = DEFAULT_SACN_PRIORITY);
};

#endif // MULTIUNIVERSE_H
