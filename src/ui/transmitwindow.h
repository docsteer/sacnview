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
class ConfigurePerChanPrioDlg;

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
    void on_btnEditPerChan_clicked();
    void on_cbPriorityMode_currentIndexChanged(int index);
    void on_sbFadersStart_valueChanged(int address);
    void on_btnCcPrev_pressed();
    void on_btnCcNext_pressed();
    void on_cbCcPap_toggled(bool checked);
    void on_lcdNumber_valueChanged(int value);
    void on_lcdNumber_toggleOff();
    void on_tabWidget_currentChanged(int index);
    void on_slChannelCheck_valueChanged(int value);
    void on_btnCcBlink_pressed();
    void on_dlFadeRate_valueChanged(int value);
    void doBlink();
    void on_sbFadeRangeStart_valueChanged(int value);
    void on_sbFadeRangeEnd_valueChanged(int value);
    void on_cbFadeRangePap_toggled(bool checked);
    void radioFadeMode_toggled(QAbstractButton *id, bool checked);
    void on_slFadeLevel_valueChanged(int value);
    void on_btnFxPause_pressed();
    void on_btnFxStart_pressed();
    void on_leScrollText_textChanged(const QString & text);
    void presetButtonPressed();
    void recordButtonPressed(bool on);
    void setLevels(QSet<int> addresses, int level);
    void dateMode_toggled(QAbstractButton* id, bool checked);
    void sourceTimeout();
    void setLevelList(QList<QPair<int, int>> levelList);
private slots:
    void on_rbDraft_clicked();

    void on_rbRatified_clicked();

    void on_sbSlotCount_valueChanged(int arg1);

    void on_rbPathwaySecure_toggled(bool checked);

    void on_sbMinFPS_editingFinished();
    void on_sbMaxFPS_editingFinished();

    void onConfigurePerChanPrioDlgFinished(int result);

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
    void updatePerChanPriorityButton();
    void updateChanCheckPap(int address);
    void updateFadeRangePap();
    Ui::transmitwindow *ui = nullptr;
    ConfigurePerChanPrioDlg *m_perChannelDialog = nullptr;
    QList<QSlider *> m_sliders;
    QList<QToolButton *> m_presetButtons;
    sACNManager::tSender m_sender;
    quint16 m_slotCount = 0;
    std::array<quint8, MAX_DMX_ADDRESS> m_perAddressPriorities = {};
    std::array<quint8, MAX_DMX_ADDRESS> m_levels = {};
    QTimer* m_blinkTimer = nullptr;
    bool m_blink = false;
    sACNEffectEngine *m_fxEngine = nullptr;
    bool m_recordMode = false;
};

#endif // TRANSMITWINDOW_H
