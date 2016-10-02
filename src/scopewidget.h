// Copyright (c) 2015 Electronic Theatre Controls, http://www.etcconnect.com
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

#ifndef SCOPEWIDGET_H
#define SCOPEWIDGET_H

#include <QWidget>
#include <QElapsedTimer>
#include "deftypes.h"

#define RING_BUF_SIZE 1000

class ScopeChannel
{
public:
    ScopeChannel();
    ScopeChannel(int universe, int address);

    int universe() { return m_universe;};
    void setUniverse(int value);

    int address() { return m_address;};
    void setAddress(int value);

    bool enabled() { return m_enabled;};
    void setEnabled(bool value) { m_enabled = value;};

    QColor color() {return m_color;};
    void setColor(const QColor &color) { m_color = color;};

    bool sixteenBit(){ return m_sixteenBit;};
    void setSixteenBit(bool value) { m_sixteenBit = value;};

    void addPoint(QPointF point);
    void clear();
    int count();
    QPointF getPoint(int index);
    qreal m_highestTime;
private:
    int m_universe;
    int m_address;
    bool m_enabled;
    QColor m_color;

    QPointF m_points[RING_BUF_SIZE];
    int m_size;
    int m_last;
    bool m_sixteenBit;
};


class ScopeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScopeWidget(QWidget *parent = 0);
    int timebase() const { return m_timebase;};
    void addChannel(ScopeChannel *channel);
    void removeChannel(ScopeChannel *channel);
    bool running() { return m_running;};
signals:

public slots:
    void setTimebase(int timebase);
    void start();
    void stop();
private slots:
    void dataReady(int address, QPointF p);
protected:
    virtual void paintEvent(QPaintEvent *event);
private:
    QList<uint1> m_points;
    QList<ScopeChannel *> m_channels;
    // The timebase in ms
    int m_timebase;
    bool m_running;
};

#endif // SCOPEWIDGET_H
