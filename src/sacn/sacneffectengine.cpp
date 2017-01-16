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

#include "sacneffectengine.h"

#include <QTimer>
#include <QThread>

sACNEffectEngine::sACNEffectEngine() : QObject(NULL)
{
    qRegisterMetaType<sACNEffectEngine::FxMode>("sACNEffectEngine::FxMode");
    m_sender = NULL;
    m_start = 0;
    m_end = 511;
    m_dateStyle = dsEU;
    m_mode = FxRamp;

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerTick()));

    m_thread = new QThread();
    moveToThread(m_thread);
    m_thread->start();
}

sACNEffectEngine::~sACNEffectEngine()
{
    m_thread->deleteLater();
    m_thread->wait();
}

void sACNEffectEngine::setSender(sACNSentUniverse *sender)
{
    m_sender = sender;
}

void sACNEffectEngine::setMode(sACNEffectEngine::FxMode mode)
{
    Q_ASSERT(QThread::currentThread()==this->thread());
    m_mode = mode;
}

void sACNEffectEngine::start()
{
    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"start");
    else
        m_timer->start();
}

void sACNEffectEngine::pause()
{
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"pause");
    else
        m_timer->stop();
}

void sACNEffectEngine::clear()
{

}

void sACNEffectEngine::setStartAddress(quint16 start)
{
    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"setStartAddress", Q_ARG(quint16, start));
    else
    {
        // Set unused values to 0
        if(start > m_start)
        {
            QMetaObject::invokeMethod(m_sender, "setLevel", Q_ARG(quint16, 0),
                                      Q_ARG(quint16, start),
                                      Q_ARG(quint8, 0));
        }
        m_start = start;
        if(m_start > m_end)
            m_end = m_start;
    }
}

void sACNEffectEngine::setEndAddress(quint16 end)
{
    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"setEndAddress", Q_ARG(quint16, end));
    else
    {
        // Set unused values to 0
        if(end < m_end)
        {
            QMetaObject::invokeMethod(m_sender, "setLevel", Q_ARG(quint16, end),
                                      Q_ARG(quint16, MAX_DMX_ADDRESS-1),
                                      Q_ARG(quint8, 0));
        }
        m_end = end;
        if(m_end < m_start)
            m_start = m_end;
    }
}

void sACNEffectEngine::setText(QString text)
{
    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
            this,"setText", Q_ARG(QString, text));
    else
    m_text = text;

}

void sACNEffectEngine::setDateStyle(DateStyle style)
{
    Q_ASSERT(QThread::currentThread()==this->thread());
    m_dateStyle = style;
}

void sACNEffectEngine::setRate(qreal hz)
{
    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
            QMetaObject::invokeMethod(
                        this,"setRate", Q_ARG(qreal, hz));
    else
    {
        int msTime = 1000 / hz;
        m_timer->setInterval(msTime);
    }
}

void sACNEffectEngine::timerTick()
{
    m_index++;

    switch(m_mode)
    {
    case FxRamp:
        m_data++;
        QMetaObject::invokeMethod(m_sender, "setLevel", Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);
        break;
    case FxSinewave:
        if(m_index >= sizeof(sinetable))
            m_index = 0;
        m_data = sinetable[m_index];
        QMetaObject::invokeMethod(m_sender, "setLevel", Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);
        break;
    case FxChase:
        if(m_index > m_end)
            m_index = m_start;
        emit setLevel(m_index, m_index, 255);
        if(m_index>m_start)
            emit setLevel(m_index-1, m_index-1, 0);
        break;

    case FxManual:
        QMetaObject::invokeMethod(m_sender, "setLevel", Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_manualLevel));
        break;
    case FxText:
    case FxDate:
        break;
    }
}



void sACNEffectEngine::setManualLevel(int level)
{
    m_manualLevel = level;
}
