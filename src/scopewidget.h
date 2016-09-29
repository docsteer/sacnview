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
    int address() { return m_address;};
    bool enabled() { return m_enabled;};
    QColor color() {return m_color;};
    void setColor(const QColor &color) { m_color = color;};

    void addPoint(QPoint point);
    void clear();
    int count();
    QPoint getPoint(int index);
    quint64 m_highestTime;
private:
    int m_universe;
    int m_address;
    bool m_enabled;
    QColor m_color;

    QPoint m_points[RING_BUF_SIZE];
    int m_size;
    int m_last;
};


class ScopeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScopeWidget(QWidget *parent = 0);
    int timebase() const { return m_timebase;};
    void addChannel(ScopeChannel *channel);
signals:

public slots:
    void setTimebase(int timebase);
    void start();
    void stop();
private slots:
    void dataReady(int address, QPoint p);
protected:
    virtual void paintEvent(QPaintEvent *event);
private:
    QList<uint1> m_points;
    QList<ScopeChannel *> m_channels;
    // The timebase in ms
    int m_timebase;
};

#endif // SCOPEWIDGET_H
