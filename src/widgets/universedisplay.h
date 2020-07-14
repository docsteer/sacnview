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

#ifndef UNIVERSEDISPLAY_H
#define UNIVERSEDISPLAY_H

#include "sacnlistener.h"
#include "gridwidget.h"
#include "consts.h"

#include <array>

class UniverseDisplay : public GridWidget
{
    Q_OBJECT
public:
    explicit UniverseDisplay(QWidget *parent = 0);
    virtual ~UniverseDisplay() {}

    static const int NO_UNIVERSE = 0;
    Q_PROPERTY(int universe READ getUniverse WRITE setUniverse NOTIFY universeChanged)
    int getUniverse() const { return m_listener ? m_listener->universe() : NO_UNIVERSE; }
    Q_SLOT void setUniverse(int universe);
    Q_SIGNAL void universeChanged();

    Q_PROPERTY(bool flickerFinder READ getFlickerFinder WRITE setFlickerFinder NOTIFY flickerFinderChanged)
    bool getFlickerFinder() const { return m_flickerFinder; }
    Q_SLOT void setFlickerFinder(bool on);
    Q_SIGNAL void flickerFinderChanged();

    Q_PROPERTY(bool showChannelPriority READ showChannelPriority WRITE setShowChannelPriority NOTIFY showChannelPriorityChanged)
    bool showChannelPriority() const { return m_showChannelPriority; }
    Q_SLOT void setShowChannelPriority(bool enable);
    Q_SIGNAL void showChannelPriorityChanged(bool enable);

public slots:
    void pause();

private slots:
    void levelsChanged();

private:
    sACNManager::tListener m_listener;
    sACNMergedSourceList m_sources;
    std::array<quint8, MAX_DMX_ADDRESS> m_flickerFinderLevels = {};
    std::array<bool, MAX_DMX_ADDRESS> m_flickerFinderHasChanged = {};
    bool m_flickerFinder = false;
    bool m_showChannelPriority = false;
};

#endif // UNIVERSEDISPLAY_H
