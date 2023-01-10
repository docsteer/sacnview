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
#include <array>

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
    static const QStringList ThemeDescriptions() {
        QStringList ret;
        ret << QObject::tr("Light Theme");
        ret << QObject::tr("Dark Theme");
        return ret;
    }

public:
    ~Preferences();

    /**
     * @brief getInstance - returns the instance of the Preferences class
     * @return the instance
     */
    static Preferences &Instance();

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
    void SetTheme(const Theme &theme);
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
    Theme GetTheme() const;
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
    QNetworkInterface m_interface;
    QHash<CID, QColor> m_cidToColor;

    QString m_sDefaultTransmitName;
    QByteArray m_mainWindowGeometry;
    QList<MDIWindowInfo> m_windowInfo;

    unsigned int m_nDisplayFormat;
    unsigned int m_nNumSecondsOfSacn;

    std::array<QByteArray, PRESET_COUNT> m_presets;
    std::array<QByteArray, PRIORITYPRESET_COUNT> m_priorityPresets;

    Theme m_theme;

    QLocale m_locale;
    quint8 m_universesListed;

    quint8 m_multicastTtl;

    QString m_pathwaySecureRxPassword;
    QString m_pathwaySecureTxPassword;

    QByteArray m_pathwaySecureSequenceMap;
    quint8 m_pathwaySecureTxSequenceType;
    quint32 m_pathwaySecureTxSequenceBootCount;
    quint32 m_pathwaySecureRxSequenceTimeWindow;

    bool m_interfaceListenAll;
    bool m_flickerFinderShowInfo;
    bool m_bBlindVisualizer;
    bool m_bETCDisplayDDOnly;
    bool m_bETCDD;
    bool m_txrateoverride;

    bool m_pathwaySecureRx;
    bool m_pathwaySecureRxDataOnly;

    bool m_saveWindowLayout;

    void loadPreferences();
};

#endif // PREFERENCES_H
