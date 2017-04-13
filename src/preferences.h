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
#include "deftypes.h"
#include "CID.h"



// Strings for storing settings
static const QString S_MAC_ADDRESS("MacAddress");
static const QString S_DISPLAY_FORMAT("Display Format");
static const QString S_BLIND_VISUALIZER("Show Blind");
static const QString S_DEFAULT_SOURCENAME("Default Transmit Source Name");
static const QString S_TIMEOUT("Timeout");
static const QString S_FLICKERFINDERSHOWINFO("Flicker Finder Info");


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
     * @return
     */
    bool defaultInterfaceAvailable();


    QColor colorForCID(const CID &cid);

    // Preferences access functions here:
    void SetDisplayFormat(unsigned int nDisplayFormat);
    void SetBlindVisualizer (bool bBlindVisualizer);
    void SetDefaultTransmitName (QString sDefaultTransmitName);
    void SetNumSecondsOfSacn (int nNumSecondsOfSacn);
    void setFlickerFinderShowInfo(bool showIt);

    unsigned int GetDisplayFormat();
    unsigned int GetMaxLevel();
    bool GetBlindVisualizer();
    QString Preferences::GetDefaultTransmitName();
    unsigned int GetNumSecondsOfSacn();
    bool getFlickerFinderShowInfo();

    QString GetFormattedValue(unsigned int nLevelInDecimal);

    void savePreferences();


    bool RESTART_APP;
public slots:
    void setNetworkInterface(const QNetworkInterface &value);
private:
    Preferences();
    ~Preferences();
    static Preferences *m_instance;
    QNetworkInterface m_interface;
    QHash<CID, QColor> m_cidToColor;

    unsigned int m_nDisplayFormat;
    bool m_bBlindVisualizer;
    QString m_sDefaultTransmitName;
    unsigned int m_nNumSecondsOfSacn;
    bool m_flickerFinderShowInfo;

    void loadPreferences();

};

#endif // PREFERENCES_H
