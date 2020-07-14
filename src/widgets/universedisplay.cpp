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

#include "universedisplay.h"
#include "preferences.h"


UniverseDisplay::UniverseDisplay(QWidget *parent)
    : GridWidget(parent)
    , m_sources(MAX_DMX_ADDRESS, sACNMergedAddress())
{
}

void UniverseDisplay::setShowChannelPriority(bool enable)
{
    if (m_showChannelPriority == enable)
        return;
    m_showChannelPriority = enable;
    emit showChannelPriorityChanged(enable);
    m_cellHeight = m_showChannelPriority ? (18 * 2) : 18;
    // Refresh
    levelsChanged();
}

void UniverseDisplay::setUniverse(int universe)
{
    // Don't search if we have already found, eg pause-continue
    if (!(m_listener && m_listener->universe() == universe))
    {
        m_listener = sACNManager::getInstance()->getListener(universe);
    }

    connect(m_listener.data(), &sACNListener::levelsChanged, this, &UniverseDisplay::levelsChanged);
    levelsChanged();
    emit universeChanged();
}

void UniverseDisplay::pause()
{
    m_listener->disconnect(this);
}

void UniverseDisplay::levelsChanged()
{
    if (!m_listener)
        return;

    const Preferences *p = Preferences::getInstance();
    m_sources = m_listener->mergedLevels();
    for (int i = 0; i < m_sources.count(); ++i)
    {
        if (m_sources[i].winningSource && i < m_sources[i].winningSource->slot_count)
        {
            QString cellText(p->GetFormattedValue(m_sources[i].level));
            if (m_showChannelPriority)
            {
                cellText.append('\n');
                cellText.append(QString::number(m_sources[i].winningSource->priority_array[i]));
            }
            setCellValue(i, cellText);

            if(m_flickerFinder)
            {
                if(m_sources[i].level > m_flickerFinderLevels[i])
                {
                    setCellColor(i, flickerHigherColor);
                    m_flickerFinderHasChanged[i] = true;
                }
                else if( m_sources[i].level < m_flickerFinderLevels[i])
                {
                    setCellColor(i, flickerLowerColor);
                    m_flickerFinderHasChanged[i] = true;
                }
                else if(m_flickerFinderHasChanged[i])
                {
                    setCellColor(i, flickerChangedColor);
                }
                else
                {
                    setCellColor(i, Qt::white);
                }
            }
            else
            {
                setCellColor(i, Preferences::getInstance()->colorForCID(m_sources[i].winningSource->src_cid));
            }
        }
        else
        {
            setCellColor(i, Qt::white);
            setCellValue(i, QString());
        }
    }
    update();
}

void UniverseDisplay::setFlickerFinder(bool on)
{
    if (!m_listener)
        return;
    m_flickerFinder = on;
    if (on)
    {
        for (int i = 0; i < MAX_DMX_ADDRESS; ++i)
        {
            m_flickerFinderLevels[i] = m_listener->mergedLevels().at(i).level;
            m_flickerFinderHasChanged[i] = false;
            setCellColor(i, Qt::white);
        }
    }
    else
    {
        levelsChanged();
    }
    update();

    emit flickerFinderChanged();
}
