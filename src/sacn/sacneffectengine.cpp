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
#include <QPainter>
#include <QFont>
#include "fontdata.h"
#include <QDateTime>

void GetCharacterCoord(unsigned char ch, int *x, int *y)
{
    *y = ch / 32;
    *x = ch % 32;
}

sACNEffectEngine::sACNEffectEngine() : QObject(NULL)
{
    qRegisterMetaType<sACNEffectEngine::FxMode>("sACNEffectEngine::FxMode");
    qRegisterMetaType<sACNEffectEngine::DateStyle>("sACNEffectEngine::DateStyle");
    m_sender = NULL;
    m_image = NULL;
    m_start = 0;
    m_end = 0;
    m_manualLevel = 0;
    m_dateStyle = dsEU;
    m_mode = FxRamp;
    m_shutdown = false;
    m_renderedImage = QImage(32, 16, QImage::Format_Grayscale8);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerTick()));

    m_thread = new QThread();
    moveToThread(m_thread);
    m_thread->start();
}

sACNEffectEngine::~sACNEffectEngine()
{
    shutdown();
}

void sACNEffectEngine::shutdown()
{
    if(!m_shutdown)
    {
        m_shutdown = true;
        m_thread->wait(5000);
        m_thread->deleteLater();
        m_thread = 0;
    }
}

void sACNEffectEngine::setSender(sACNSentUniverse *sender)
{
    m_sender = sender;
}

void sACNEffectEngine::setMode(sACNEffectEngine::FxMode mode)
{
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"setMode", Q_ARG(sACNEffectEngine::FxMode, mode));
    else
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
            QMetaObject::invokeMethod(m_sender, "setLevelRange", Q_ARG(quint16, 0),
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
            QMetaObject::invokeMethod(m_sender, "setLevelRange", Q_ARG(quint16, end),
                                      Q_ARG(quint16, MAX_DMX_ADDRESS-1),
                                      Q_ARG(quint8, 0));
        }
        m_end = end;
        if(m_end < m_start)
            m_start = m_end;
    }
}


void sACNEffectEngine::renderText(QString text)
{
    if(m_image)
        delete m_image;
    m_imageWidth = 8 * text.length();
    int img_size = 16 * m_imageWidth;
    m_image = new uint1[img_size];

    memset(m_image, 0, img_size);
    renderText(text, 4, true);
}


void sACNEffectEngine::renderText(QString top, QString bottom)
{
    if(m_image)
        delete m_image;
    m_imageWidth = 32;
    int img_size = 16 * m_imageWidth;
    m_image = new uint1[img_size];
    memset(m_image, 0, img_size);

    renderText(top, 1, false);
    renderText(bottom, 9, false);
}

void sACNEffectEngine::renderText(QString text, int yStart, bool big)
{
    Q_ASSERT(m_image);
    int char_width = big ? 8 : 4;
    int char_height = big  ? 8 : 5;

    int width = char_width * text.length();
    int height = 16;

    int img_size = width * height;


    for (int i = 0 ; i < text.length() ; i++)
    {
        unsigned char c = text.at(i).toLatin1();
        unsigned char *character_font;
        if(big)
            character_font = vincent_data[c];
        else
            character_font = minifont_data[c];

        int base_x = 0 + i * char_width;
        int base_y = yStart;


        for (int y = 0 ; y < char_height ; y++)
        {
            char character_scanline = character_font[y];
            for (int x = 0 ; x < char_width ; x++)
            {
                int raw_pixel = (1 << (8 - 1 - x)) & character_scanline;
                bool pixel = raw_pixel > 0 ? true : false;
                int pixel_index = (base_x + x) + (base_y + y)*width;

                if(pixel == true && pixel_index < img_size) {
                    m_image[pixel_index] = 255;
                }
            }
        }
    }



    m_renderedImage = QImage(m_image, width, height, QImage::Format_Grayscale8);

    emit textImageChanged(QPixmap::fromImage(m_renderedImage.scaledToHeight(100)));
}

void sACNEffectEngine::setText(QString text)
{
    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
            this,"setText", Q_ARG(QString, text));
    else
    {
        m_text = text;

        renderText(text);
    }

}

void sACNEffectEngine::setDateStyle(sACNEffectEngine::DateStyle style)
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
        m_rate = hz;
        int msTime = 1000 / hz;
        m_timer->setInterval(msTime);
    }
}

void sACNEffectEngine::timerTick()
{
    m_index++;
    char line[32];

    if(m_shutdown)
    {
        m_timer->stop();
        delete m_timer;
    }

    switch(m_mode)
    {
    case FxRamp:
        m_data++;
        QMetaObject::invokeMethod(m_sender, "setLevelRange", Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);
        break;
    case FxSinewave:
        if(m_index >= sizeof(sinetable))
            m_index = 0;
        m_data = sinetable[m_index];
        QMetaObject::invokeMethod(m_sender, "setLevelRange", Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);
        break;
    case FxChase:
        if(m_index > m_end)
            m_index = m_start;
        QMetaObject::invokeMethod(m_sender, "setLevel", Q_ARG(quint16, m_index),
                                  Q_ARG(quint8, 255));
        if(m_index>m_start)
            QMetaObject::invokeMethod(m_sender, "setLevel", Q_ARG(quint16, m_index-1),
                                      Q_ARG(quint8, 0));
        break;

    case FxManual:
        QMetaObject::invokeMethod(m_sender, "setLevelRange", Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_manualLevel));
        break;
    case FxText:
        if(m_image)
        {
            if(m_index > m_imageWidth) m_index=0;

            for(int i=0; i<16; i++)
            {
                // Rolling window on to the image
                quint8 *scanline = m_image + (i*m_imageWidth);
                memset(line, 0, 32);
                for(int j=0; j<32; j++)
                {
                    if(m_imageWidth>0) // Protect against empty text
                        line[j] = scanline[(m_index + j) % m_imageWidth  ];
                }

                m_sender->setLevel((quint8*)&line, 32, i*32);
            }
        }
        break;
    case FxDate:
        if(m_dateStyle==dsEU)
            renderText(QDateTime::currentDateTime().toString("hh:mm:ss"),
                   QDateTime::currentDateTime().toString("dd/MM/yy"));
        else
            renderText(QDateTime::currentDateTime().toString("hh:mm:ss"),
                   QDateTime::currentDateTime().toString("MM/dd/yy"));
        for(int i=0; i<16; i++)
        {
            quint8 *scanline = m_image + (i*32);
            m_sender->setLevel(scanline, 32, i*32);
        }
        break;
    }
}



void sACNEffectEngine::setManualLevel(int level)
{
    m_manualLevel = level;

    QMetaObject::invokeMethod(m_sender, "setLevelRange", Q_ARG(quint16, m_start),
                              Q_ARG(quint16, m_end),
                              Q_ARG(quint8, m_manualLevel));
}
