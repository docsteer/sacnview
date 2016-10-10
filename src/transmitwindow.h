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
#include "deftypes.h"
#include "consts.h"

class sACNSentUniverse;

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
    void on_tabWidget_currentChanged(int index);
    void on_slChannelCheck_valueChanged(int value);
    void on_btnCcBlink_pressed();
    void doBlink();
protected:
    virtual void keyPressEvent(QKeyEvent *event);
private:
    enum TABS
    {
        tabSliders,
        tabChannelCheck,
        tabFadeRange,
        tabChase,
        tabText,
        tabDate
    };

    void setUniverseOptsEnabled(bool enabled);

    Ui::transmitwindow *ui;
    QList<QSlider *> m_sliders;
    QList<QLabel *> m_sliderLabels;
    sACNSentUniverse *m_sender;
    uint1 m_perAddressPriorities[MAX_DMX_ADDRESS];
    uint1 m_levels[MAX_DMX_ADDRESS];
    QTimer *m_blinkTimer;
    bool m_blink;
};

#endif // TRANSMITWINDOW_H
