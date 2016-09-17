
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

#include <Qt>
#include <QSettings>
#include "preferences.h"
#include "consts.h"

// The base color to generate pastel shades for sources
static const QColor mixColor = QColor("coral");

Preferences *Preferences::m_instance = NULL;

Preferences::Preferences()
{
    loadPreferences();
}

Preferences::~Preferences()
{
    savePreferences();
}

Preferences *Preferences::getInstance()
{
    if(!m_instance)
    {
        m_instance = new Preferences();
    }

    return m_instance;
}

QNetworkInterface Preferences::networkInterface() const
{
    return m_interface;
}

void Preferences::setNetworkInterface(const QNetworkInterface &value)
{
    if(m_interface.name()!=value.name())
    {
        m_interface = value;
        //emit networkInterfaceChanged();
    }
}

QColor Preferences::colorForCID(const CID &cid)
{
    if(m_cidToColor.contains(cid))
        return m_cidToColor[cid];

    int red = qrand() % 255;
    int green = qrand() % 255;
    int blue = qrand() % 255;

    red = (red + mixColor.red()) / 2;
    green = (green + mixColor.green()) / 2;
    blue = (blue + mixColor.blue()) / 2;

    QColor newColor = QColor::fromRgb(red, green, blue);
    m_cidToColor[cid] = newColor;
    return newColor;
}


void Preferences::SetDisplayFormat(unsigned int nDisplayFormat)
{
    Q_ASSERT(nDisplayFormat < TOTAL_NUM_OF_FORMATS);
    m_nDisplayFormat = nDisplayFormat;
    return;
}

QString Preferences::GetFormattedValue(unsigned int nLevelInDecimal)
{
    Q_ASSERT(nLevelInDecimal<=255);
    if (m_nDisplayFormat == DECIMAL)
        return QString::number(nLevelInDecimal, 10);
    else if (m_nDisplayFormat == PERCENT)
        return QString::number(HTOPT[nLevelInDecimal], 10);
    else if (m_nDisplayFormat == HEXADECIMAL)
        return QString::number(nLevelInDecimal, 16);
    else
        return QString ("Err");

}

void Preferences::SetBlindVisualizer (bool bBlindVisualizer)
{
    Q_ASSERT(bBlindVisualizer == 0 || bBlindVisualizer == 1);
    m_bBlindVisualizer = bBlindVisualizer;
    return;
}

void Preferences::SetNumSecondsOfSacn (int nNumSecondsOfSacn)
{
    Q_ASSERT(nNumSecondsOfSacn >= 0 && nNumSecondsOfSacn <= MAX_SACN_TRANSMIT_TIME_SEC);
    m_nNumSecondsOfSacn = nNumSecondsOfSacn;
    return;
}

unsigned int Preferences::GetDisplayFormat()
{
    return m_nDisplayFormat;
}

bool Preferences::GetBlindVisualizer()
{
    return m_bBlindVisualizer;
}

unsigned int Preferences::GetNumSecondsOfSacn()
{
   return m_nNumSecondsOfSacn;
}

bool Preferences::defaultInterfaceAvailable()
{
    return m_interface.isValid();
}

void Preferences::savePreferences()
{
    QSettings settings;

    if(m_interface.isValid())
        settings.setValue(S_MAC_ADDRESS, m_interface.hardwareAddress());
    settings.setValue(S_DISPLAY_FORMAT, QVariant(m_nDisplayFormat));
    settings.setValue(S_BLIND_VISUALIZER, QVariant(m_bBlindVisualizer));
    settings.setValue(S_TIMEOUT, QVariant(m_nNumSecondsOfSacn));
}

void Preferences::loadPreferences()
{
    QSettings settings;

    if(settings.contains(S_MAC_ADDRESS))
    {
        QString mac = settings.value(S_MAC_ADDRESS).toString();
        QList<QNetworkInterface> ifaceList = QNetworkInterface::allInterfaces();
        foreach(QNetworkInterface i, ifaceList)
            if(i.hardwareAddress() == mac)
                m_interface = i;
    }

    m_nDisplayFormat = settings.value(S_DISPLAY_FORMAT, QVariant(DECIMAL)).toInt();
    m_bBlindVisualizer = settings.value(S_BLIND_VISUALIZER, QVariant(false)).toBool();
    m_nNumSecondsOfSacn = settings.value(S_TIMEOUT, QVariant(0)).toInt();
}
