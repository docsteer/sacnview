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


static const QColor flickerHigherColor  = QColor::fromRgb(0x8d, 0x32, 0xfd);
static const QColor flickerLowerColor   = QColor::fromRgb(0x04, 0xfd, 0x44);
static const QColor flickerChangedColor = QColor::fromRgb(0xfb, 0x09, 0x09);

UniverseDisplay::UniverseDisplay(QWidget *parent) : GridWidget(parent)
{
    for(int i=0; i<512; i++)
        m_sources << sACNMergedAddress();
    memset(m_flickerFinderLevels, 0, MAX_DMX_ADDRESS);
    memset(m_flickerFinderHasChanged, 0, MAX_DMX_ADDRESS);
    m_flickerFinder = false;
}

void UniverseDisplay::setUniverse(int universe)
{
    m_listener = sACNManager::getInstance()->getListener(universe);
    connect(m_listener.data(), SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));
    levelsChanged();
}

void UniverseDisplay::pause()
{
    m_listener->disconnect(this);
}

void UniverseDisplay::levelsChanged()
{
    if(!m_listener)
        return;

    Preferences *p = Preferences::getInstance();
    m_sources = m_listener->mergedLevels();
    for(int i=0; i<m_sources.count(); i++)
    {
        if(m_sources[i].winningSource)
        {
            setCellValue(i, p->GetFormattedValue(m_sources[i].level));

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
            setCellValue(i, QString());
    }
    update();
}

void UniverseDisplay::setFlickerFinder(bool on)
{
    m_flickerFinder = on;
    if(on)
    {
        for(int i=0; i<MAX_DMX_ADDRESS; i++)
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
}
