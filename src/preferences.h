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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QObject>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QColor>
#include <QTranslator>
#include "CID.h"
#include "consts.h"



// Strings for storing settings
static const QString S_MAC_ADDRESS("MacAddress");
static const QString S_DISPLAY_FORMAT("Display Format");
static const QString S_BLIND_VISUALIZER("Show Blind");
static const QString S_DDONLY("Show DDOnly");
static const QString S_DEFAULT_SOURCENAME("Default Transmit Source Name");
static const QString S_TIMEOUT("Timeout");
static const QString S_FLICKERFINDERSHOWINFO("Flicker Finder Info");
static const QString S_SAVEWINDOWLAYOUT("Save Window Layout");
static const QString S_PRESETS("Preset %1");
static const QString S_MAINWINDOWGEOM("Main Window Geometry");
static const QString S_SUBWINDOWLIST("Sub Window");
static const QString S_SUBWINDOWNAME("SubWindow Name");
static const QString S_SUBWINDOWGEOM("SubWindow Geometry");
static const QString S_LISTEN_ALL("Listen All");
static const QString S_THEME("Theme");
static const QString S_TRANSLATION("Translation Filename");

struct MDIWindowInfo
{
    QString name;
    QByteArray geometry;
};

class Preferences
{

public:
    enum DisplayFormats
        {
            DECIMAL = 0,
            PERCENT = 1,
            HEXADECIMAL = 2,
            TOTAL_NUM_OF_FORMATS = 3
        };

    enum Theme {
        THEME_LIGHT,
        THEME_DARK,
        TOTAL_NUM_OF_THEMES
    };
    static const QStringList ThemeDescriptions;


    /**
     * @brief getInstance - returns the instance of the Preferences class
     * @return the instance
     */
    static Preferences *getInstance();

    /**
     * @brief networkInterface returns the user preferred network interface for multicast
     * @return the network interface to use
     */
    QNetworkInterface networkInterface() const;

    /**
     * @brief defaultInterfaceAvailable - returns whether the default interface selected by the user
     * is available
     * @return true/false
     */
    bool defaultInterfaceAvailable();

    /**
     * @brief interfaceSuitable - returns whether the interface is suitable for sACN
     * @param inter - the interface to check
     * @return true/false
     */
    bool interfaceSuitable(QNetworkInterface *inter);


    QColor colorForCID(const CID &cid);

    // Preferences access functions here:
    void SetDisplayFormat(unsigned int nDisplayFormat);
    void SetBlindVisualizer (bool bBlindVisualizer);
    void SetDisplayDDOnly (bool bDDOnly);
    void SetDefaultTransmitName (QString sDefaultTransmitName);
    void SetNumSecondsOfSacn (int nNumSecondsOfSacn);
    void setFlickerFinderShowInfo(bool showIt);
    void SetPreset(const QByteArray &data, int index);
    void SetSaveWindowLayout(bool value);
    void SetMainWindowGeometry(const QByteArray &value);
    void SetSavedWindows(QList<MDIWindowInfo> values);
    void SetNetworkListenAll(const bool &value);
    void SetTheme(Theme theme);
    void SetTranslationFilename(QString filename);

    unsigned int GetDisplayFormat();
    unsigned int GetMaxLevel();
    bool GetBlindVisualizer();
    bool GetDisplayDDOnly();
    QString GetDefaultTransmitName();
    unsigned int GetNumSecondsOfSacn();
    bool getFlickerFinderShowInfo();
    QByteArray GetPreset(int index);
    bool GetSaveWindowLayout();
    QByteArray GetMainWindowGeometry();
    QList<MDIWindowInfo> GetSavedWindows();
    bool GetNetworkListenAll();
    Theme GetTheme();
    QString GetTranslationFilename();

    QTranslator* CurrentTranslator = Q_NULLPTR;

    QString GetFormattedValue(unsigned int nLevelInDecimal, bool decorated = false);

    void savePreferences();

    bool RESTART_APP;
public slots:
    void setNetworkInterface(const QNetworkInterface &value);
private:
    Preferences();
    ~Preferences();
    static Preferences *m_instance;
    QNetworkInterface m_interface;
    bool m_interfaceListenAll;
    QHash<CID, QColor> m_cidToColor;

    unsigned int m_nDisplayFormat;
    bool m_bBlindVisualizer;
    bool m_bDisplayDDOnly;
    QString m_sDefaultTransmitName;
    unsigned int m_nNumSecondsOfSacn;
    bool m_flickerFinderShowInfo;
    QByteArray m_presets[MAX_DMX_ADDRESS];
    bool m_saveWindowLayout;
    QByteArray m_mainWindowGeometry;
    QList<MDIWindowInfo> m_windowInfo;
    Theme m_theme;
    QString m_translationFilename;

    void loadPreferences();
};

#endif // PREFERENCES_H
