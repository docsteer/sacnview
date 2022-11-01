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

#ifndef SCOPEWIDGET_H
#define SCOPEWIDGET_H

#include <QWidget>
#include <QElapsedTimer>

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

    enum TriggerMode
    {
        tmNormal,
        tmRisingEdge,
        tmFallingEdge
    };

    explicit ScopeWidget(QWidget *parent = 0);
    int timebase() const { return m_timebase;};
    void addChannel(ScopeChannel *channel);
    void removeChannel(ScopeChannel *channel);
    bool running() { return m_running;};

    void setTriggerMode(TriggerMode mode);
    void setTriggerAddress(int universe, int channel);
    void setTriggerThreshold(int value);
signals:

public slots:
    void setTimebase(int timebase);
    void setTriggerDelay(int triggerDelay);
    void start();
    void stop();
private slots:
    void dataReady(int address, const QPointF data);
signals:
    void stopped();
protected:
    virtual void paintEvent(QPaintEvent *event);
private:
    QList<quint8> m_points;
    QList<ScopeChannel *> m_channels;
    // The timebase in ms
    int m_timebase;
    bool m_running;
    bool m_triggered;
    TriggerMode m_triggerMode;
    int m_triggerUniverse, m_triggerChannel;
    int m_triggerLevel;
    int m_triggerDelay;
    int m_endTriggerTime;
};

#endif // SCOPEWIDGET_H
