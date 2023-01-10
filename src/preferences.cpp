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

#include <Qt>
#include <QSettings>
#include <QPalette>
#include <QStyle>
#include <QApplication>
#include "preferences.h"
#include "consts.h"

// The base color to generate pastel shades for sources
static const QColor mixColor = QColor("coral");

Preferences *Preferences::m_instance = Q_NULLPTR;

Preferences::Preferences()
{
    RESTART_APP = false;
    for(int i=0; i<PRESET_COUNT; i++)
        m_presets[i] = QByteArray(MAX_DMX_ADDRESS, char(0));
    for(int i=0; i<PRIORITYPRESET_COUNT; i++)
        m_priorityPresets[i] = QByteArray(MAX_DMX_ADDRESS, char(100+i));
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
    if (!m_interface.isValid())
    {
        for (QNetworkInterface interface : QNetworkInterface::allInterfaces())
        {
            if (interface.flags().testFlag(QNetworkInterface::IsLoopBack))
                return interface;
        }
    }
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

void Preferences::SetNetworkListenAll(const bool &value)
{
    if (networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack)) return;
    m_interfaceListenAll = value;
}

bool Preferences::GetNetworkListenAll()
{
#ifdef TARGET_WINXP
    return false;
#else
    if (networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack)) return false;
    return m_interfaceListenAll;
#endif
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

QString Preferences::GetFormattedValue(unsigned int nLevelInDecimal, bool decorated)
{
    Q_ASSERT(nLevelInDecimal<=255);
    QString result;
    if (m_nDisplayFormat == DECIMAL)
    {
        result = QString::number(nLevelInDecimal, 10);
    }
    else if (m_nDisplayFormat == PERCENT)
    {
        result = QString::number(HTOPT[nLevelInDecimal], 10);
        if(decorated) result += QString("%");
    }
    else if (m_nDisplayFormat == HEXADECIMAL)
    {
        if(decorated) result = QString("0x");
        result = QString::number(nLevelInDecimal, 16);
    }
    else
        result = QString ("Err");

    return result;
}

void Preferences::SetBlindVisualizer (bool bBlindVisualizer)
{
    Q_ASSERT(bBlindVisualizer == 0 || bBlindVisualizer == 1);
    m_bBlindVisualizer = bBlindVisualizer;
    return;
}

void Preferences::SetDisplayDDOnly(bool bDDOnly)
{
    Q_ASSERT(bDDOnly == 0 || bDDOnly == 1);
    m_bDisplayDDOnly = bDDOnly;
    return;
}

void Preferences::SetDefaultTransmitName (QString sDefaultTransmitName)
{
    sDefaultTransmitName.truncate(MAX_SOURCE_NAME_LEN);
    m_sDefaultTransmitName = sDefaultTransmitName.trimmed();
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

unsigned int Preferences::GetMaxLevel()
{
    if(m_nDisplayFormat==PERCENT)
        return 100;
    return MAX_SACN_LEVEL;
}

bool Preferences::GetBlindVisualizer()
{
    return m_bBlindVisualizer;
}

bool Preferences::GetDisplayDDOnly()
{
    return m_bDisplayDDOnly;
}

QString Preferences::GetDefaultTransmitName()
{
   return m_sDefaultTransmitName;
}

unsigned int Preferences::GetNumSecondsOfSacn()
{
   return m_nNumSecondsOfSacn;
}

bool Preferences::defaultInterfaceAvailable()
{
    return interfaceSuitable(&m_interface);
}

bool Preferences::interfaceSuitable(QNetworkInterface *inter)
{

    if (inter->isValid() && (
                (
                    // Up, can multicast...
                    inter->flags().testFlag(QNetworkInterface::IsRunning)
                    && inter->flags().testFlag(QNetworkInterface::IsUp)
                    && inter->flags().testFlag(QNetworkInterface::CanMulticast)
                ) || (
                    // Up, is loopback...
                    inter->flags().testFlag(QNetworkInterface::IsRunning)
                    && inter->flags().testFlag(QNetworkInterface::IsUp)
                    && inter->flags().testFlag(QNetworkInterface::IsLoopBack)
                )
            )
        )
    {
        // ...has IPv4
        foreach (QNetworkAddressEntry addr, inter->addressEntries()) {
            if(addr.ip().protocol() == QAbstractSocket::IPv4Protocol)
               return true;
        }
    }
    return false;
}

void Preferences::SetTheme(Theme theme)
{
    m_theme = theme;
}

Preferences::Theme Preferences::GetTheme()
{
    return m_theme;
}

void Preferences::savePreferences()
{
    QSettings settings;

    if(m_interface.isValid())
    {
        settings.setValue(S_INTERFACE_ADDRESS, m_interface.hardwareAddress());
        settings.setValue(S_INTERFACE_NAME, m_interface.name());
    }
    settings.setValue(S_DISPLAY_FORMAT, QVariant(m_nDisplayFormat));
    settings.setValue(S_BLIND_VISUALIZER, QVariant(m_bBlindVisualizer));
    settings.setValue(S_DDONLY, QVariant(m_bDisplayDDOnly));
    settings.setValue(S_DEFAULT_SOURCENAME, m_sDefaultTransmitName);
    settings.setValue(S_TIMEOUT, QVariant(m_nNumSecondsOfSacn));
    settings.setValue(S_FLICKERFINDERSHOWINFO, QVariant(m_flickerFinderShowInfo));
    settings.setValue(S_SAVEWINDOWLAYOUT, m_saveWindowLayout);
    settings.setValue(S_MAINWINDOWGEOM, m_mainWindowGeometry);
    settings.setValue(S_LISTEN_ALL, m_interfaceListenAll);
    settings.setValue(S_THEME, m_theme);
    settings.setValue(S_TX_RATE_OVERRIDE, m_txrateoverride);
    settings.setValue(S_LOCALE, m_locale);
    settings.setValue(S_UNIVERSESLISTED, m_universesListed);

    settings.beginWriteArray(S_SUBWINDOWLIST);
    for(int i=0; i<m_windowInfo.count(); i++)
    {
        settings.setArrayIndex(i);
        settings.setValue(S_SUBWINDOWNAME, m_windowInfo[i].name);
        settings.setValue(S_SUBWINDOWGEOM, m_windowInfo[i].geometry);
    }
    settings.endArray();

    for(int i=0; i<PRESET_COUNT; i++)
    {
        settings.setValue(S_PRESETS.arg(i), QVariant(m_presets[i]));
    }

    for(int i=0; i<PRIORITYPRESET_COUNT; i++)
    {
        settings.setValue(S_PRIORITYPRESET.arg(i), QVariant(m_priorityPresets[i]));
    }

    settings.setValue(S_MULTICASTTTL, m_multicastTtl);

    settings.sync();
}

void Preferences::loadPreferences()
{
    QSettings settings;

    if(settings.contains(S_INTERFACE_ADDRESS))
    {
        QString mac = settings.value(S_INTERFACE_ADDRESS).toString();
        auto interFromName = QNetworkInterface::interfaceFromName(settings.value(S_INTERFACE_NAME).toString());
        if (interFromName.isValid() && (interFromName.hardwareAddress() == mac))
        {
            m_interface = interFromName;
        } else {
            QList<QNetworkInterface> ifaceList = QNetworkInterface::allInterfaces();
            for(auto i : ifaceList)
                if(i.hardwareAddress() == mac)
                    m_interface = i;
        }
    }

    m_interfaceListenAll = settings.value(S_LISTEN_ALL, QVariant(false)).toBool();
    m_nDisplayFormat = settings.value(S_DISPLAY_FORMAT, QVariant(DECIMAL)).toInt();
    m_bBlindVisualizer = settings.value(S_BLIND_VISUALIZER, QVariant(true)).toBool();
    m_bDisplayDDOnly = settings.value(S_DDONLY, QVariant(true)).toBool();
    m_sDefaultTransmitName = settings.value(S_DEFAULT_SOURCENAME, DEFAULT_SOURCE_NAME).toString();
    m_nNumSecondsOfSacn = settings.value(S_TIMEOUT, QVariant(0)).toInt();
    m_flickerFinderShowInfo = settings.value(S_FLICKERFINDERSHOWINFO, QVariant(true)).toBool();
    m_saveWindowLayout = settings.value(S_SAVEWINDOWLAYOUT, QVariant(false)).toBool();
    m_mainWindowGeometry = settings.value(S_MAINWINDOWGEOM, QVariant(QByteArray())).toByteArray();
    m_theme = (Theme) settings.value(S_THEME, QVariant((int)THEME_LIGHT)).toInt();
    m_txrateoverride = settings.value(S_TX_RATE_OVERRIDE, QVariant(false)).toBool();
    m_locale = settings.value(S_LOCALE, QLocale::system()).toLocale();
    m_universesListed = settings.value(S_UNIVERSESLISTED, QVariant(20)).toUInt();
    m_multicastTtl = settings.value(S_MULTICASTTTL, QVariant(1)).toUInt();

    m_windowInfo.clear();
    int size = settings.beginReadArray(S_SUBWINDOWLIST);
    for(int i=0; i<size; i++)
    {
        MDIWindowInfo value;
        settings.setArrayIndex(i);
        value.name = settings.value(S_SUBWINDOWNAME).toString();
        value.geometry = settings.value(S_SUBWINDOWGEOM).toByteArray();
        m_windowInfo << value;
    }
    settings.endArray();

    for(int i=0; i<PRESET_COUNT; i++)
    {
        if(settings.contains(S_PRESETS.arg(i)))
        {
            m_presets[i] = settings.value(S_PRESETS.arg(i)).toByteArray();
        }
    }


    for(int i=0; i<PRIORITYPRESET_COUNT; i++)
    {
        if(settings.contains(S_PRIORITYPRESET.arg(i)))
        {
            m_priorityPresets[i] = settings.value(S_PRIORITYPRESET.arg(i)).toByteArray();
        }
    }
}

void Preferences::setFlickerFinderShowInfo(bool showIt)
{
    m_flickerFinderShowInfo = showIt;
}

bool Preferences::getFlickerFinderShowInfo()
{
    return m_flickerFinderShowInfo;
}

void Preferences::SetPreset(const QByteArray &data, int index)
{
    Q_ASSERT(index>=0);
    Q_ASSERT(index<PRESET_COUNT);

    m_presets[index] = data;
}

QByteArray Preferences::GetPreset(int index)
{
    Q_ASSERT(index>=0);
    Q_ASSERT(index<PRESET_COUNT);

    return m_presets[index];
}

void Preferences::SetSaveWindowLayout(bool value)
{
    m_saveWindowLayout = value;
}

bool Preferences::GetSaveWindowLayout()
{
    return m_saveWindowLayout;
}

void Preferences::SetMainWindowGeometry(const QByteArray &value)
{
    m_mainWindowGeometry = value;
}

QByteArray Preferences::GetMainWindowGeometry()
{
    return m_mainWindowGeometry;
}

void Preferences::SetSavedWindows(QList<MDIWindowInfo> values)
{
    m_windowInfo = values;
}

QList<MDIWindowInfo> Preferences::GetSavedWindows()
{
    return m_windowInfo;
}

void Preferences::SetLocale(QLocale locale)
{
    m_locale = locale;
}

QLocale Preferences::GetLocale()
{
    return m_locale;
}

void Preferences::SetPriorityPreset(const QByteArray &data, int index)
{
    Q_ASSERT(index < PRIORITYPRESET_COUNT);
    m_priorityPresets[index] = data;
}

QByteArray Preferences::GetPriorityPreset(int index)
{
    Q_ASSERT(index < PRIORITYPRESET_COUNT);
    return m_priorityPresets[index];
}
