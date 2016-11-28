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
    m_sender = NULL;
    m_start = 0;
    m_end = 511;
    m_dateStyle = dsEU;
    m_mode = FxFadeRamp;

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerTick()));

    m_thread = new QThread();
    moveToThread(m_thread);
    m_thread->start();
}

void sACNEffectEngine::setSender(sACNSentUniverse *sender)
{
    if(m_sender)
        disconnect(m_sender, 0, this, 0);

    m_sender = sender;
    connect(this, SIGNAL(setLevel(uint2,uint1)), sender, SLOT(setLevel(uint2, uint1)));
    connect(this, SIGNAL(setLevel(uint2,uint2,uint1)), sender, SLOT(setLevel(uint2,uint2,uint1)));

}

void sACNEffectEngine::setMode(FxMode mode)
{
    m_mode = mode;
}

void sACNEffectEngine::start()
{
    m_timer->start();
}

void sACNEffectEngine::pause()
{
    m_timer->stop();
}

void sACNEffectEngine::clear()
{

}

void sACNEffectEngine::setStartAddress(uint2 start)
{
    m_start = start;
}

void sACNEffectEngine::setEndAddress(uint2 end)
{
    m_end = end;
}

void sACNEffectEngine::setText(QString text)
{
    m_text = text;

}

void sACNEffectEngine::setDateStyle(DateStyle style)
{
    m_dateStyle = style;
}

void sACNEffectEngine::setRate(qreal hz)
{
    int msTime = 1000 / hz;
    m_timer->setInterval(msTime);
}

void sACNEffectEngine::timerTick()
{
    m_index++;

    switch(m_mode)
    {
    case FxFadeRamp:
        if(m_index > m_end)
            m_index = m_start;
        m_data++;
        emit setLevel(m_start, m_end, m_data);
        break;
    case FxFadeSinewave:
        if(m_index > m_end)
            m_index = m_start;
        m_data = sinetable[m_index];
        emit setLevel(m_start, m_end, m_data);
    case FxChase:
        if(m_index > m_end)
            m_index = m_start;
        emit setLevel(m_index, m_index, 255);
        if(m_index>m_start)
            emit setLevel(m_index-1, m_index-1, 0);
        break;
    case FxText:
    case FxDate:
        break;
    }
}
