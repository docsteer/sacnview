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

UniverseDisplay::UniverseDisplay(QWidget *parent) : GridWidget(parent)
{
    m_listener = 0;
    for(int i=0; i<512; i++)
        m_sources << sACNMergedAddress();
}

void UniverseDisplay::setUniverse(int universe)
{
    m_listener = sACNManager::getInstance()->getListener(universe);
    connect(m_listener, SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));
}

void UniverseDisplay::pause()
{
    m_listener->disconnect(this);
    m_listener = NULL;
}

void UniverseDisplay::levelsChanged()
{
    if(m_listener)
    {
        Preferences *p = Preferences::getInstance();
        m_sources = m_listener->mergedLevels();
        for(int i=0; i<m_sources.count(); i++)
        {
            if(m_sources[i].winningSource)
            {
                setCellValue(i, p->GetFormattedValue(m_sources[i].level));

                setCellColor(i, Preferences::getInstance()->colorForCID(m_sources[i].winningSource->src_cid));
            }
            else
                setCellValue(i, QString());
        }
        update();
    }
}

