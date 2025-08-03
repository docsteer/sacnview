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

#include "sacneffectengine.h"

#include <QTimer>
#include <QThread>
#include <QPainter>
#include <QFont>
#include "fontdata.h"
#include <QDateTime>
#include <QEventLoop>

void GetCharacterCoord(unsigned char ch, int *x, int *y)
{
    *y = ch / 32;
    *x = ch % 32;
}

sACNEffectEngine::sACNEffectEngine(sACNManager::tSender sender) : QObject(NULL),
  m_sender(sender.data()),
  m_mode(FxManual),
  m_dateStyle(dsEU),
  m_start(0),
  m_end(0),
  m_index(0),
  m_index_chase(0),
  m_data(0),
  m_manualLevel(0),
  m_renderedImage(QImage(32, 16, QImage::Format_Grayscale8)),
  m_image(Q_NULLPTR)
{
    qRegisterMetaType<sACNEffectEngine::FxMode>("sACNEffectEngine::FxMode");
    qRegisterMetaType<sACNEffectEngine::DateStyle>("sACNEffectEngine::DateStyle");

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::TimerType::PreciseTimer);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &sACNEffectEngine::timerTick);

    m_thread = new QThread();
    moveToThread(m_thread);
    connect(m_thread, &QThread::finished, this, &QObject::deleteLater);
    m_thread->setObjectName(QString("Effect Engine Universe %1").arg(sender->universe()));
    m_thread->start();

    connect(m_sender, &sACNSentUniverse::slotCountChange, this, &sACNEffectEngine::slotCountChanged);
}

sACNEffectEngine::~sACNEffectEngine()
{
    // Stop thread
    m_thread->quit();
}

void sACNEffectEngine::setMode(sACNEffectEngine::FxMode mode)
{
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"setMode", Q_ARG(sACNEffectEngine::FxMode, mode));
    else
        m_mode = mode;

    clear();
}

void sACNEffectEngine::run()
{
    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"run");
    else
    {
        m_timer->start();
        emit running();
    }
}

void sACNEffectEngine::pause()
{
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"pause");
    else
    {
        m_timer->stop();
        emit paused();
    }
}

void sACNEffectEngine::clear()
{
    QMetaObject::invokeMethod(
                m_sender,"setLevelRange",
                Q_ARG(quint16, MIN_DMX_ADDRESS - 1),
                Q_ARG(quint16, m_sender->slotCount() - 1),
                Q_ARG(quint8, 0));
}

void sACNEffectEngine::clearUnused()
{
    quint16 start = std::max(std::min(m_start, m_end), static_cast<quint16>(MIN_DMX_ADDRESS - 1));
    quint16 end = std::min(std::max(m_start, m_end), static_cast<quint16>(MAX_DMX_ADDRESS - 1));

    if (start > static_cast<quint16>(MIN_DMX_ADDRESS - 1))
        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, MIN_DMX_ADDRESS - 1),
                                  Q_ARG(quint16, start - 1),
                                  Q_ARG(quint8, 0));

    if (end < static_cast<quint16>(MAX_DMX_ADDRESS - 1))
        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, end + 1),
                                  Q_ARG(quint16, MAX_DMX_ADDRESS - 1),
                                  Q_ARG(quint8, 0));
}

void sACNEffectEngine::setStartAddress(quint16 start)
{

    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"setStartAddress", Q_ARG(quint16, start));
    else
    {
        // Limit to sender slot count
        m_start = std::min(start, m_sender->slotCount());

        // Set unused values to 0
        clearUnused();
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
        // Limit to sender slot count
        m_end = std::min(end, m_sender->slotCount());

        // Set unused values to 0
        clearUnused();
    }
}

void sACNEffectEngine::setRange(quint16 start, quint16 end)
{
    Q_ASSERT(start < DMX_SLOT_MAX);
    Q_ASSERT(end < DMX_SLOT_MAX);

    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
        QMetaObject::invokeMethod(
                    this,"setRange", Q_ARG(quint16, start), Q_ARG(quint16, end));
    else
    {
        if(end < start)
            std::swap(start, end);

        setStartAddress(start);
        setEndAddress(end);
    }
}


void sACNEffectEngine::renderText(QString text)
{
    if(m_image)
        delete m_image;
    m_imageWidth = 8 * text.length();
    const auto img_size = 16 * m_imageWidth;
    m_image = new quint8[img_size];

    memset(m_image, 0, img_size);
    renderText(text, 4, true);
}


void sACNEffectEngine::renderText(QString top, QString bottom)
{
    if(m_image)
        delete m_image;
    m_imageWidth = 32;
    const auto img_size = 16 * m_imageWidth;
    m_image = new quint8[img_size];
    memset(m_image, 0, img_size);

    renderText(top, 1, false);
    renderText(bottom, 9, false);
}

void sACNEffectEngine::renderText(QString text, int yStart, bool big)
{
    Q_ASSERT(m_image);
    int char_width = big ? 8 : 4;
    int char_height = big  ? 8 : 5;

    const auto width = char_width * text.length();
    int height = 16;

    const auto img_size = width * height;


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
                const auto pixel_index = (base_x + x) + (base_y + y)*width;

                if(pixel == true && pixel_index < img_size) {
                    m_image[pixel_index] = 255;
                }
            }
        }
    }



    m_renderedImage = QImage(m_image, static_cast<int>(width), static_cast<int>(height), QImage::Format_Grayscale8);

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
    // Make this method thread-safe
    if(QThread::currentThread()!=this->thread())
            QMetaObject::invokeMethod(
                        this,"setDateStyle", Q_ARG(sACNEffectEngine::DateStyle, style));
    else
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

    switch(m_mode)
    {
    case FxRamp:
        m_data++;
        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);
        break;
    case FxInverseRamp:
        m_data--;
        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);
        break;
    case FxSinewave:
        if(m_index >= sizeof(sinetable))
            m_index = 0;
        m_data = sinetable[m_index];
        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);
        break;
    case FxChaseSnap:
        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, 0));
        QMetaObject::invokeMethod(m_sender, "setLevel",
                                  Q_ARG(quint16, m_index_chase),
                                  Q_ARG(quint8, m_manualLevel));

        if (m_index_chase < m_start)
            m_index_chase = m_start;

        if(++m_index_chase > m_end)
            m_index_chase = m_start;

        break;

    case FxChaseRamp:
        if(m_index > std::numeric_limits<decltype(m_data)>::max())
        {
            m_index = std::numeric_limits<decltype(m_data)>::min();
            if (++m_index_chase > m_end )
                m_index_chase = m_start;
        }

        if (m_index_chase < m_start)
            m_index_chase = m_start;

        m_data = m_index;

        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, 0));
        QMetaObject::invokeMethod(m_sender, "setLevel",
                                  Q_ARG(quint16, m_index_chase),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);


        break;

    case FxChaseSine:
        if(m_index >= sizeof(sinetable))
        {
            m_index = 0;
            if (++m_index_chase > m_end )
                m_index_chase = m_start;
        }

        if (m_index_chase < m_start)
            m_index_chase = m_start;

        m_data = sinetable[m_index];

        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, m_start),
                                  Q_ARG(quint16, m_end),
                                  Q_ARG(quint8, 0));
        QMetaObject::invokeMethod(m_sender, "setLevel",
                                  Q_ARG(quint16, m_index_chase),
                                  Q_ARG(quint8, m_data));
        emit fxLevelChange(m_data);


        break;

    case FxManual:
        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                  Q_ARG(quint16, m_start),
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
    case FxVerticalBar:
        if(m_index > 31)
            m_index = 0;
        QMetaObject::invokeMethod(m_sender, "setVerticalBar",
                                    Q_ARG(quint16, m_index),
                                    Q_ARG(quint8, 255));
        break;
    case FxHorizontalBar:
        if(m_index > 15)
            m_index = 0;
        QMetaObject::invokeMethod(m_sender, "setHorizontalBar",
                                    Q_ARG(quint16, m_index),
                                    Q_ARG(quint8, 255));
        break;
    }
}

void sACNEffectEngine::setManualLevel(int level)
{
    m_manualLevel = level;

    if (m_mode == FxManual) {
        QMetaObject::invokeMethod(m_sender, "setLevelRange",
                                      Q_ARG(quint16, m_start),
                                      Q_ARG(quint16, m_end),
                                      Q_ARG(quint8, m_manualLevel));
    }
}

void sACNEffectEngine::slotCountChanged()
{
    setEndAddress(m_end);
}
