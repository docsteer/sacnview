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
#include "deftypes.h"
#include "consts.h"

class sACNSentUniverse;
class sACNEffectEngine;

namespace Ui {
class transmitwindow;
}

class transmitwindow : public QWidget
{
    Q_OBJECT

public:
    explicit transmitwindow(QWidget *parent = 0);
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
    void on_sbFadersStart_valueChanged(int value);
    void on_btnCcPrev_pressed();
    void on_btnCcNext_pressed();
    void on_lcdNumber_valueChanged(int value);
    void on_tabWidget_currentChanged(int index);
    void on_slChannelCheck_valueChanged(int value);
    void on_btnCcBlink_pressed();
    void on_dlFadeRate_valueChanged(int value);
    void doBlink();
    void on_sbFadeRangeStart_valueChanged(int value);
    void on_sbFadeRangeStart_editingFinished();
    void on_sbFadeRangeEnd_valueChanged(int value);
    void on_sbFadeRangeEnd_editingFinished();
    void radioFadeMode_toggled(bool checked);
    void on_slFadeLevel_valueChanged(int value);
    void on_btnFxPause_pressed();
    void on_btnFxStart_pressed();
    void on_leScrollText_textChanged(const QString & text);
    void presetButtonPressed();
    void recordButtonPressed(bool on);
    void setLevels(QSet<int> addresses, int level);
    void dateMode_toggled(bool checked);
    void sourceTimeout();
private slots:
    void on_rbDraft_clicked();

    void on_rbRatified_clicked();

private:
    enum TABS
    {
        tabSliders,
        tabChannelCheck,
        tabEffects,
    };

    void setUniverseOptsEnabled(bool enabled);

    Ui::transmitwindow *ui;
    QList<QSlider *> m_sliders;
    QList<QLabel *> m_sliderLabels;
    QList<QToolButton *> m_presetButtons;
    sACNSentUniverse *m_sender;
    uint1 m_perAddressPriorities[MAX_DMX_ADDRESS];
    uint1 m_levels[MAX_DMX_ADDRESS];
    QTimer *m_blinkTimer;
    bool m_blink;
    sACNEffectEngine *m_fxEngine;
    bool m_recordMode;
};

#endif // TRANSMITWINDOW_H
