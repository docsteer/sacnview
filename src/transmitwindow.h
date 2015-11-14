// Copyright (c) 2015 Tom Barthel-Steer, http://www.tomsteer.net
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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

protected slots:
    void fixSize();
    void on_btnStart_pressed();
    void on_sbUniverse_valueChanged(int value);
    void on_sliderMoved(int value);
    void on_btnEditPerChan_pressed();
    void on_cbPriorityMode_currentIndexChanged(int index);
private:
    void setUniverseOptsEnabled(bool enabled);

    Ui::transmitwindow *ui;
    QList<QSlider *> m_sliders;
    QList<QLabel *> m_sliderLabels;
    sACNSentUniverse *m_sender;
    uint1 m_perAddressPriorities[MAX_DMX_ADDRESS];
};

#endif // TRANSMITWINDOW_H
