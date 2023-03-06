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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QObject>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QColor>
#include <QLocale>
#include "CID.h"
#include "consts.h"
#include "themes.h"

// Strings for storing settings
static const QString S_INTERFACE_ADDRESS("MacAddress");
static const QString S_INTERFACE_NAME("InterfaceName");
static const QString S_DISPLAY_FORMAT("Display Format");
static const QString S_BLIND_VISUALIZER("Show Blind");
static const QString S_ETC_DDONLY("Show ETC DD Only");
static const QString S_ETC_DD("Enable ETC DD");
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
static const QString S_TX_RATE_OVERRIDE("TX Rate Override");
static const QString S_LOCALE("LOCALE");
static const QString S_UNIVERSESLISTED("Universe List Count");
static const QString S_PRIORITYPRESET("PriorityPreset %1");
static const QString S_MULTICASTTTL("Multicast TTL");
static const QString S_PATHWAYSECURE_RX("Enable Pathway Secure Rx");
static const QString S_PATHWAYSECURE_RX_PASSWORD("Pathway Secure Rx Password");
static const QString S_PATHWAYSECURE_TX_PASSWORD("Pathway Secure Tx Password");
static const QString S_PATHWAYSECURE_RX_DATA_ONLY("Show Pathway Secure RX Data Only");
static const QString S_PATHWAYSECURE_RX_SEQUENCE_TIME_WINDOW("Pathway Secure Data RX Sequence Time Window");
static const QString S_PATHWAYSECURE_TX_SEQUENCE_TYPE("Pathway Secure Data TX Sequence Type");
static const QString S_PATHWAYSECURE_TX_SEQUENCE_BOOT_COUNT("Pathway Secure Data TX Sequence Boot Count");
static const QString S_PATHWAYSECURE_SEQUENCE_MAP("Pathway Secure Data Sequence Map");
static const QString S_UPDATE_IGNORE("Ignore Update Version");

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
    bool defaultInterfaceAvailable() const;

    /**
     * @brief interfaceSuitable - returns whether the interface is suitable for sACN
     * @param inter - the interface to check
     * @return true/false
     */
    bool interfaceSuitable(const QNetworkInterface &iface) const;

    static QString GetIPv4AddressString(const QNetworkInterface &inter);

    QColor colorForCID(const CID &cid);

    // Preferences access functions here:
    void SetDisplayFormat(unsigned int nDisplayFormat);
    void SetBlindVisualizer (bool bBlindVisualizer);
    void SetETCDisplayDDOnly (bool bETCDDOnly);
    void SetETCDD(bool bETCDD);
    void SetDefaultTransmitName (const QString &sDefaultTransmitName);
    void SetNumSecondsOfSacn (int nNumSecondsOfSacn);
    void setFlickerFinderShowInfo(bool showIt);
    void SetPreset(const QByteArray &data, int index);
    void SetSaveWindowLayout(bool value);
    void SetMainWindowGeometry(const QByteArray &value);
    void SetSavedWindows(const QList<MDIWindowInfo> &values);
    void SetNetworkListenAll(const bool &value);
    void SetTheme(Themes::theme_e theme);
    void SetTXRateOverride(bool override) { m_txrateoverride = override; }
    void SetLocale(const QLocale &locale);
    void SetUniversesListed(quint8 count) { m_universesListed = (std::max)(count, (quint8)1); }
    void SetPriorityPreset(const QByteArray &data, int index);
    void SetMulticastTtl(quint8 ttl) { m_multicastTtl = ttl;}
    void SetPathwaySecureRx(bool enable);
    void SetPathwaySecureRxPassword(QString password);
    void SetPathwaySecureTxPassword(QString password);
    void SetPathwaySecureRxDataOnly(bool value);
    void SetPathwaySecureTxSequenceType(quint8 type);
    void SetPathwaySecureRxSequenceTimeWindow(quint32 value);
    void SetPathwaySecureRxSequenceBootCount(quint32 value);
    void SetPathwaySecureSequenceMap(QByteArray map);
    void SetUpdateIgnore(QString version);

    unsigned int GetDisplayFormat() const;
    unsigned int GetMaxLevel() const;
    bool GetBlindVisualizer() const;
    bool GetETCDisplayDDOnly() const;
    bool GetETCDD() const;
    QString GetDefaultTransmitName() const;
    unsigned int GetNumSecondsOfSacn() const;
    bool getFlickerFinderShowInfo() const;
    QByteArray GetPreset(int index) const;
    bool GetSaveWindowLayout() const;
    QByteArray GetMainWindowGeometry() const;
    QList<MDIWindowInfo> GetSavedWindows() const;
    bool GetNetworkListenAll() const;
    Themes::theme_e GetTheme() const;
    bool GetTXRateOverride() const { return m_txrateoverride; }
    QLocale GetLocale() const;
    quint8 GetUniversesListed() const { return m_universesListed; }
    quint8 GetMulticastTtl() const { return m_multicastTtl; }
    bool GetPathwaySecureRx() const;
    QString GetPathwaySecureRxPassword() const;
    QString GetPathwaySecureTxPassword() const;
    bool GetPathwaySecureRxDataOnly() const;
    quint8 GetPathwaySecureTxSequenceType() const;
    quint32 GetPathwaySecureRxSequenceTimeWindow() const;
    quint32 GetPathwaySecureRxSequenceBootCount() const;
    QByteArray GetPathwaySecureSequenceMap() const;
    QString GetUpdateIgnore() const;

    QString GetFormattedValue(unsigned int nLevelInDecimal, bool decorated = false) const;
    QByteArray GetPriorityPreset(int index) const;
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
    bool m_bETCDisplayDDOnly;
    bool m_bETCDD;
    QString m_sDefaultTransmitName;
    unsigned int m_nNumSecondsOfSacn;
    bool m_flickerFinderShowInfo;
    QByteArray m_presets[PRESET_COUNT];
    bool m_saveWindowLayout;
    QByteArray m_mainWindowGeometry;
    QList<MDIWindowInfo> m_windowInfo;
    Themes::theme_e m_theme;
    bool m_txrateoverride;
    QLocale m_locale;
    quint8 m_universesListed;
    QByteArray m_priorityPresets[PRIORITYPRESET_COUNT];
    quint8 m_multicastTtl;
    bool m_pathwaySecureRx;
    QString m_pathwaySecureRxPassword;
    QString m_pathwaySecureTxPassword;
    bool m_pathwaySecureRxDataOnly;
    quint8 m_pathwaySecureTxSequenceType;
    quint32 m_pathwaySecureTxSequenceBootCount;
    quint32 m_pathwaySecureRxSequenceTimeWindow;
    QByteArray m_pathwaySecureSequenceMap;

    void loadPreferences();
};

#endif // PREFERENCES_H
