// Copyright (c) 2015 Tom Barthel-Steer, http://www.tomsteer.net
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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

