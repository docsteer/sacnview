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

#ifndef TRANSMITWINDOW_H
#define TRANSMITWINDOW_H

#include <QWidget>
#include <QtGui>
#include <QLabel>
#include <QSlider>
#include <QToolButton>
#include <array>
#include "consts.h"
#include "streamingacn.h"

class sACNSentUniverse;
class sACNEffectEngine;

namespace Ui {
class transmitwindow;
}

class transmitwindow : public QWidget
{
    Q_OBJECT

public:
    explicit transmitwindow(int universe = MIN_SACN_UNIVERSE, QWidget *parent = Q_NULLPTR);
    ~transmitwindow();
    static const int BLINK_TIME = 1000;
    static const int NUM_SLIDERS = 24;
protected slots:
    void fixSize();
    void on_btnStart_pressed();
    void on_sbUniverse_valueChanged(int value);
    void on_sliderMoved(int value);
    void on_btnEditPerChan_pressed();
    void on_cbPriorityMode_currentIndexChanged(int index);
    void on_sbFadersStart_valueChanged(int address);
    void on_btnCcPrev_pressed();
    void on_btnCcNext_pressed();
    void on_lcdNumber_valueChanged(int value);
    void on_lcdNumber_toggleOff();
    void on_tabWidget_currentChanged(int index);
    void on_slChannelCheck_valueChanged(int value);
    void on_btnCcBlink_pressed();
    void on_dlFadeRate_valueChanged(int value);
    void doBlink();
    void on_sbFadeRangeStart_valueChanged(int value);
    void on_sbFadeRangeEnd_valueChanged(int value);
    void radioFadeMode_toggled(int id, bool checked);
    void on_slFadeLevel_valueChanged(int value);
    void on_btnFxPause_pressed();
    void on_btnFxStart_pressed();
    void on_leScrollText_textChanged(const QString & text);
    void presetButtonPressed();
    void recordButtonPressed(bool on);
    void setLevels(QSet<int> addresses, int level);
    void dateMode_toggled(int id, bool checked);
    void sourceTimeout();
    void setLevelList(QList<QPair<int, int>> levelList);
private slots:
    void on_rbDraft_clicked();

    void on_rbRatified_clicked();

    void on_sbSlotCount_valueChanged(int arg1);

private:
    enum TABS
    {
        tabSliders,
        tabChannelCheck,
        tabEffects,
        tabGrid
    };

    void setUniverseOptsEnabled(bool enabled);
    void updateTitle();
    void setLevel(int address, int value);
    Ui::transmitwindow *ui;
    QList<QSlider *> m_sliders;
    QList<QToolButton *> m_presetButtons;
    sACNManager::tSender m_sender;
    quint16 m_slotCount;
    std::array<quint8, MAX_DMX_ADDRESS> m_perAddressPriorities;
    std::array<quint8, MAX_DMX_ADDRESS> m_levels;
    QTimer *m_blinkTimer;
    bool m_blink;
    sACNEffectEngine *m_fxEngine;
    bool m_recordMode;
};

#endif // TRANSMITWINDOW_H
