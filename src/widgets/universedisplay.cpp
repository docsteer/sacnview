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

#include "universedisplay.h"
#include "preferences.h"

#include <QTimerEvent>

UniverseDisplay::UniverseDisplay(QWidget * parent)
    : GridWidget(parent), m_sources(MAX_DMX_ADDRESS, sACNMergedAddress())
{}

void UniverseDisplay::setShowChannelPriority(bool enable)
{
    if (m_showChannelPriority == enable) return;
    m_showChannelPriority = enable;
    emit showChannelPriorityChanged(enable);
    updateCellHeight();
    // Refresh
    levelsChanged();
}

void UniverseDisplay::setStableCompareTime(qint64 milliseconds)
{
    if (m_stableCompareTime == milliseconds) return;

    m_stableCompareTime = milliseconds;

    updateUniverseCompareTimer();

    emit stableCompareTimeChanged();
}

const QColor & UniverseDisplay::flickerHigherColor()
{
    switch (Preferences::Instance().GetTheme())
    {
        default:
        case Themes::LIGHT:
        {
            static const QColor highLight(0x8d, 0x32, 0xfd);
            return highLight;
        }
        case Themes::DARK: static const QColor highDark(0x46, 0x19, 0x7e); return highDark;
    }
}

const QColor & UniverseDisplay::flickerLowerColor()
{
    switch (Preferences::Instance().GetTheme())
    {
        default:
        case Themes::LIGHT:
        {
            static const QColor lowLight(0x04, 0xfd, 0x44);
            return lowLight;
        }
        case Themes::DARK:
        {
            static const QColor lowDark(0x02, 0x7e, 0x22);
            return lowDark;
        }
    }
}

const QColor & UniverseDisplay::flickerChangedColor()
{
    switch (Preferences::Instance().GetTheme())
    {
        default:
        case Themes::LIGHT:
        {
            static const QColor changeLight(0xfb, 0x09, 0x09);
            return changeLight;
        }
        case Themes::DARK:
        {
            static const QColor changeDark(0x7d, 0x04, 0x04);
            return changeDark;
        }
    }
}

void UniverseDisplay::setUniverse(int universe)
{
    // Don't search if we have already found, eg pause-continue
    if (!(m_listener && m_listener->universe() == universe))
    {
        m_listener = sACNManager::Instance().getListener(universe);
    }

    connect(m_listener.data(), &sACNListener::levelsChanged, this, &UniverseDisplay::levelsChanged);

    // Restart comparison
    setCompareToUniverse(getCompareToUniverse());

    levelsChanged();
    emit universeChanged();
}

void UniverseDisplay::pause()
{
    m_listener->disconnect(this);
    if (m_compareListener) m_compareListener->disconnect(this);
}

void UniverseDisplay::levelsChanged()
{
    if (!m_listener) return;

    if (m_compareListener)
    {
        // Mark approx timestamp of main level changes
        const qint64 timestamp = sACNManager::elapsed();
        sACNMergedSourceList newSources = m_listener->mergedLevels();

        for (size_t i = 0; i < newSources.size(); ++i)
        {
            if (newSources[i].level != m_sources[i].level) m_compareTimestamp[i] = timestamp;
        }
        std::swap(newSources, m_sources);

        updateUniverseCompare();
        return;
    }

    m_sources = m_listener->mergedLevels();

    const Preferences & pref = Preferences::Instance();
    for (int i = 0; i < m_sources.count(); ++i)
    {
        if (m_sources[i].winningSource && i < m_sources[i].winningSource->slot_count)
        {
            QString cellText(pref.GetFormattedValue(m_sources[i].level));
            if (m_showChannelPriority)
            {
                cellText.append('\n');
                cellText.append(QString::number(m_sources[i].winningSource->priority_array[i]));
            }

            if (m_flickerFinder)
            {
                if (m_sources[i].level > m_compareLevels[i])
                {
                    setCellColor(i, flickerHigherColor());
                    m_compareDifference[i] = 1;
                }
                else if (m_sources[i].level < m_compareLevels[i])
                {
                    setCellColor(i, flickerLowerColor());
                    m_compareDifference[i] = 1;
                }
                else if (m_compareDifference[i] != 0)
                {
                    setCellColor(i, flickerChangedColor());
                }
            }
            else
            {
                setCellColor(i, Preferences::Instance().colorForCID(m_sources[i].winningSource->src_cid));
            }

            setCellValue(i, cellText);
        }
        else
        {
            setCellColor(i, Preferences::Instance().GetTheme() == Themes::LIGHT ? Qt::white : Qt::black);
            setCellValue(i, QString());
        }
    }
    update();
}

void UniverseDisplay::compareLevelsChanged()
{
    if (!m_compareListener) return;

    std::array<int, MAX_DMX_ADDRESS> newLevels = m_compareListener->mergedLevelsOnly();
    const qint64 timestamp = sACNManager::elapsed();

    // Mark approx timestamp of compare level changes
    for (size_t i = 0; i < newLevels.size(); ++i)
    {
        if (newLevels[i] != m_compareLevels[i]) m_compareTimestamp[i] = timestamp;
    }

    // Swap and update
    std::swap(m_compareLevels, newLevels);
    updateUniverseCompare();
}

void UniverseDisplay::timerEvent(QTimerEvent * ev)
{
    if (ev->timerId() == m_compareTimer)
    {
        updateUniverseCompare();
    }
}

void UniverseDisplay::updateCellHeight()
{
    m_cellHeight = m_showChannelPriority ? (18 * 2) : 18;
    if (m_compareListener) m_cellHeight += 18;
}

void UniverseDisplay::updateUniverseCompare()
{
    const Preferences & pref = Preferences::Instance();
    const qint64 nowElapsed = sACNManager::elapsed();

    // Compare slots if the level has been static for long enough
    for (int i = 0; i < m_sources.count(); ++i)
    {
        QString cellText(pref.GetFormattedValue(m_sources[i].level));
        cellText.append('\n');
        cellText.append(pref.GetFormattedValue(m_compareLevels[i]));

        if ((m_compareTimestamp[i] + m_stableCompareTime) < nowElapsed)
        {
            // Set the cell colours for differences
            if (m_sources[i].level > m_compareLevels[i])
            {
                setCellColor(i, flickerHigherColor());
            }
            else if (m_sources[i].level < m_compareLevels[i])
            {
                setCellColor(i, flickerLowerColor());
            }
        }

        setCellValue(i, cellText);
    }
    update();
}

void UniverseDisplay::updateUniverseCompareTimer()
{
    // Reset timer
    if (m_compareTimer != 0)
    {
        killTimer(m_compareTimer);
        m_compareTimer = 0;
    }

    // Start looking for static out-of-sync levels if appropriate
    if (m_compareListener) m_compareTimer = startTimer(static_cast<int>(m_stableCompareTime / 4));
}

void UniverseDisplay::setFlickerFinder(bool on)
{
    if (!m_listener) return;

    if (!on && !m_flickerFinder) return;

    m_flickerFinder = on;
    if (on)
    {
        // Can't do both
        setCompareToUniverse(NO_UNIVERSE);

        m_compareLevels = m_listener->mergedLevelsOnly();
        m_compareDifference.fill(0);
        setAllCellColor(Preferences::Instance().GetTheme() == Themes::LIGHT ? Qt::white : Qt::black);
    }
    else
    {
        levelsChanged();
    }
    update();

    emit flickerFinderChanged();
}

void UniverseDisplay::setCompareToUniverse(int otherUniverse)
{
    if (!m_listener) return;

    if (otherUniverse == NO_UNIVERSE)
    {
        if (getCompareToUniverse() == NO_UNIVERSE) return;

        m_compareListener.clear();
        levelsChanged();
    }
    else
    {
        // Can't do both
        setFlickerFinder(false);

        m_compareLevels.fill(-1);
        m_compareDifference.fill(0);

        setAllCellColor(Preferences::Instance().GetTheme() == Themes::LIGHT ? Qt::white : Qt::black);
        m_compareListener = sACNManager::Instance().getListener(otherUniverse);
        connect(m_compareListener.data(), &sACNListener::levelsChanged, this, &UniverseDisplay::compareLevelsChanged);
        // Was it already running?
        if (m_compareListener->sourceCount() > 0) compareLevelsChanged();
    }

    updateUniverseCompareTimer();
    updateCellHeight();
    update();

    emit compareToUniverseChanged();
}
