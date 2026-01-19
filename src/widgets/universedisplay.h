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

#ifndef UNIVERSEDISPLAY_H
#define UNIVERSEDISPLAY_H

#include "consts.h"
#include "gridwidget.h"
#include "sacnlistener.h"

#include <array>

class UniverseDisplay : public GridWidget
{
    Q_OBJECT
public:

    explicit UniverseDisplay(QWidget * parent = 0);
    virtual ~UniverseDisplay() {}

    static constexpr int NO_UNIVERSE = 0;
    Q_PROPERTY(int universe READ getUniverse WRITE setUniverse NOTIFY universeChanged)
    int getUniverse() const { return m_listener ? m_listener->universe() : NO_UNIVERSE; }
    Q_SLOT void setUniverse(int universe);
    Q_SIGNAL void universeChanged();

    Q_PROPERTY(bool flickerFinder READ getFlickerFinder WRITE setFlickerFinder NOTIFY flickerFinderChanged)
    bool getFlickerFinder() const { return m_flickerFinder; }
    Q_SLOT void setFlickerFinder(bool on);
    Q_SIGNAL void flickerFinderChanged();

    Q_PROPERTY(
        bool showChannelPriority READ showChannelPriority WRITE setShowChannelPriority NOTIFY
            showChannelPriorityChanged)
    bool showChannelPriority() const { return m_showChannelPriority; }
    Q_SLOT void setShowChannelPriority(bool enable);
    Q_SIGNAL void showChannelPriorityChanged(bool enable);

    Q_PROPERTY(
        int compareToUniverse READ getCompareToUniverse WRITE setCompareToUniverse NOTIFY compareToUniverseChanged)
    int getCompareToUniverse() const { return m_compareListener ? m_compareListener->universe() : NO_UNIVERSE; }
    Q_SLOT void setCompareToUniverse(int otherUniverse);
    Q_SIGNAL void compareToUniverseChanged();

    Q_PROPERTY(
        qint64 stableCompareTime READ getStableCompareTime WRITE setStableCompareTime NOTIFY stableCompareTimeChanged)
    qint64 getStableCompareTime() const { return m_stableCompareTime; }
    Q_SLOT void setStableCompareTime(qint64 milliseconds);
    Q_SIGNAL void stableCompareTimeChanged();

    static const QColor & flickerHigherColor();
    static const QColor & flickerLowerColor();
    static const QColor & flickerChangedColor();

public slots:
    void pause();

private slots:
    void levelsChanged();
    void compareLevelsChanged();

protected:

    void timerEvent(QTimerEvent * ev) override;

private:

    void updateCellHeight();
    void updateUniverseCompare();
    void updateUniverseCompareTimer();

private:

    sACNManager::tListener m_listener;
    sACNMergedSourceList m_sources;
    // Flicker and delta finder backing stores
    sACNManager::tListener m_compareListener;
    std::array<int, MAX_DMX_ADDRESS> m_compareLevels = {}; // Level to compare against
    std::array<int, MAX_DMX_ADDRESS> m_compareDifference = {}; // Difference marker
    std::array<qint64, MAX_DMX_ADDRESS> m_compareTimestamp = {}; // Difference timestamp
    qint64 m_stableCompareTime = 1000; // Milliseconds level must be stable before considered different
    int m_compareTimer = 0;
    bool m_flickerFinder = false;
    bool m_showChannelPriority = false;
};

#endif // UNIVERSEDISPLAY_H
