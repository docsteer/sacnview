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

#include "sacnlistener.h"
#include "gridwidget.h"
#include "consts.h"

class UniverseDisplay : public GridWidget
{
    Q_OBJECT
public:
    explicit UniverseDisplay(QWidget *parent = 0);
    virtual ~UniverseDisplay() {}
    bool flickerFinder() const { return m_flickerFinder; }

    Q_PROPERTY(bool showChannelPriority READ showChannelPriority WRITE setShowChannelPriority NOTIFY showChannelPriorityChanged)
    bool showChannelPriority() const { return m_showChannelPriority; }
    Q_SLOT void setShowChannelPriority(bool enable);
    Q_SIGNAL void showChannelPriorityChanged(bool enable);

public slots:
    void setUniverse(int universe);
    void levelsChanged();
    void pause();
    void setFlickerFinder(bool on);
private:
    sACNMergedSourceList m_sources;
    sACNManager::tListener m_listener;
    quint8 m_flickerFinderLevels[MAX_DMX_ADDRESS];
    bool m_flickerFinderHasChanged[MAX_DMX_ADDRESS];
    bool m_flickerFinder;
    bool m_showChannelPriority;
};

#endif // UNIVERSEDISPLAY_H
