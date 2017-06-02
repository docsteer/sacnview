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

#ifndef SACNLISTENER_H
#define SACNLISTENER_H

#include <QObject>
#include <vector>
#include <list>
#include <QTimer>
#include <QElapsedTimer>
#include <QPoint>
#include "streamingacn.h"
#include "sacnsocket.h"


struct sACNMergedAddress
{
    sACNMergedAddress() {
        level = -1;
        winningSource = NULL;
        changedSinceLastMerge = false;
    }
    int level;
    sACNSource *winningSource;
    QSet<sACNSource *> otherSources;
    bool changedSinceLastMerge;
};

typedef QList<sACNMergedAddress> sACNMergedSourceList;

/**
 * @brief The sACNListener class is used to listen to  a universe of sACN.
 * The class should not be instantiated directly; instead, instead use sACNManager to get the listener for a universe - this
 * allows reuse of listeners
 */
class sACNListener : public QObject
{
    Q_OBJECT
public:
    sACNListener(QObject *parent = 0);
    virtual ~sACNListener();

    /**
     * @brief universe
     * @return the universe which this listener is listening for
     */
    int universe() {return m_universe;}
    /**
     * @brief mergedLevels
     * @return an sACNMergerdSourceList, a list of merged address structures, allowing you to see
     * the result of the merge algorithm together with all the sub-sources, by address
     */
    sACNMergedSourceList mergedLevels() { return m_merged_levels;}

    int sourceCount() { return m_sources.size();}
    sACNSource *source(int index) { return m_sources[index];}

    // Diagnostic - the number of merge operations per second
    int mergesPerSecond() { return (m_mergesPerSecond > 0) ? m_mergesPerSecond : 0;}
public slots:
    void startReception(int universe);
    void monitorAddress(int address) { m_monitoredChannels.insert(address);}
    void unMonitorAddress(int address) { m_monitoredChannels.remove(address);}
signals:
    void sourceFound(sACNSource *source);
    void sourceLost(sACNSource *source);
    void sourceChanged(sACNSource *source);
    void levelsChanged();
    void dataReady(int address, QPointF data);
private slots:
    void readPendingDatagrams();
    void performMerge();
    void checkSourceExpiration();
    void checkSampleExpiration();
private:
    std::list<sACNRxSocket *> m_sockets;
    std::vector<sACNSource *> m_sources;
    int m_last_levels[512];
    sACNMergedSourceList m_merged_levels;
    int m_universe;
    // The per-source hold last look time
    int m_ssHLL;
    // Are we in the initial sampling state
    bool m_isSampling;
    ttimer m_sampleTimer;
    QTimer *m_initalSampleTimer;
    QTimer *m_mergeTimer;
    QElapsedTimer m_elapsedTime;
    int m_predictableTimerValue;
    QSet<int> m_monitoredChannels;
    bool m_mergeAll; // A flag to initiate a complete remerge of everything
    int m_mergesPerSecond;
    int m_mergeCounter;
    QElapsedTimer m_mergesPerSecondTimer;
};


#endif // SACNLISTENER_H
