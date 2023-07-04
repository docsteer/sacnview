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

#ifndef SACNLISTENER_H
#define SACNLISTENER_H

#include <QObject>
#include <QThread>
#include <vector>
#include <list>
#include <QTimer>
#include <QElapsedTimer>
#include <QPoint>
#include "consts.h"
#include "streamingacn.h"
#include "sacnsocket.h"

#include <array>

Q_DECLARE_METATYPE(QHostAddress)

struct sACNMergedAddress
{
  sACNMergedAddress() = default;
  sACNSource* winningSource = nullptr;
  QSet<sACNSource*> otherSources;
  int level = -1;
  int winningPriority = 0;
  bool changedSinceLastMerge = false;
};

typedef QVector<sACNMergedAddress> sACNMergedSourceList;

/**
 * @brief The sACNListener class is used to listen to  a universe of sACN.
 * The class should not be instantiated directly; instead, instead use sACNManager to get the listener for a universe - this
 * allows reuse of listeners
 */
class sACNListener : public QObject
{
  Q_OBJECT

public:
  class IDmxReceivedCallback
  {
  public:
    virtual ~IDmxReceivedCallback() {}
    /**
    * @brief Called in the listener thread after a DMX packet has been received and merged
    * @param packet_tock global tock of this packet
    * @param universe number
    * @param mergedLevels array of merged levels
    */
    virtual void sACNListenerDmxReceived(tock packet_tock, int universe, const std::array<int, MAX_DMX_ADDRESS> &mergedLevels) = 0;
  };

public:
  sACNListener(int universe, QObject* parent = 0);
  virtual ~sACNListener();

  /**
   * @brief universe
   * @return the universe which this listener is listening for
   */
  int universe() const { return m_universe; }

  /**
   * @brief mergedLevels
   * @return an sACNMergedSourceList, a list of merged address structures, allowing you to see
   * the result of the merge algorithm together with all the sub-sources, by address
   */
  sACNMergedSourceList mergedLevels() {
    QMutexLocker mergeLocker(&m_merged_levelsMutex);
    return m_merged_levels;
  }

  /**
 * @brief mergedLevels
 * @param address The address to return
 * @return an sACNMergedAddress containing the result of the merge algorithm together with all the sub-sources
 */
  sACNMergedAddress mergedLevel(int address) {
    if (address < 0 || address >= m_merged_levels.size()) return sACNMergedAddress();
    QMutexLocker mergeLocker(&m_merged_levelsMutex);
    return m_merged_levels[address];
  }

  /**
 * @brief mergedLevelsOnly
 * @return an array of merged levels. -1 means no source at all
 */
  std::array<int, MAX_DMX_ADDRESS> mergedLevelsOnly() {
    QMutexLocker mergeLocker(&m_merged_levelsMutex);
    return m_current_levels;
  }

  /**
 * @brief mergedPrioritiesOnly
 * @return an array of final priorities. -1 means no source at all
 */
  std::array<int, MAX_DMX_ADDRESS> mergedPrioritiesOnly() {
    QMutexLocker mergeLocker(&m_merged_levelsMutex);
    return m_current_priorities;
  }

  int sourceCount() const { return static_cast<int>(m_sources.size()); }
  sACNSource* source(int index) { return m_sources[index]; }
  const std::vector<sACNSource*> getSourceList() const { return m_sources; }

  /**
   *  @brief processDatagram Process a suspected sACN datagram.
   * This allows other listeners to pass on unicast datagrams for other universes
   *
   */
  Q_INVOKABLE void processDatagram(const QByteArray& data, const QHostAddress& destination, const QHostAddress& sender);

  // Diagnostic - the number of merge operations per second

  int mergesPerSecond() const { return (m_mergesPerSecond > 0) ? m_mergesPerSecond : 0; }

  /**
   *  @brief getBindStatus Get interface bind status of listener
   *  @return A struct of bind types and status
   */
  sACNRxSocket::sBindStatus getBindStatus() const { return m_bindStatus; }

  /**
   * @brief Force the listener to perform a full merge
   */
  void doFullMerge() { m_mergeAll = true; }

  // Objects that want a direct callback for levels in the listener thread
  void addDirectCallback(IDmxReceivedCallback* callback);
  void removeDirectCallback(IDmxReceivedCallback* callback);

public slots:
  void startReception();

signals:
  void listenerStarted(int universe);
  void sourceFound(sACNSource* source);
  void sourceLost(sACNSource* source);
  void sourceResumed(sACNSource* source);
  void sourceChanged(sACNSource* source);
  void levelsChanged();

private slots:
  void readPendingDatagrams();
  void performMerge();
  void checkSourceExpiration();
  void sampleExpiration();

private:
  QMutex m_processMutex;
  void startInterface(const QNetworkInterface& iface);
  std::list<sACNRxSocket*> m_sockets;
  std::vector<sACNSource*> m_sources;
  std::array<int, MAX_DMX_ADDRESS> m_last_levels = {};
  std::array<int, MAX_DMX_ADDRESS> m_last_priorities = {};
  std::array<int, MAX_DMX_ADDRESS> m_current_levels = {};
  std::array<int, MAX_DMX_ADDRESS> m_current_priorities = {};
  sACNMergedSourceList m_merged_levels;
  QMutex m_merged_levelsMutex;
  const int m_universe = 0;
  // The per-source hold last look time
  int m_ssHLL = 1000;
  // Are we in the initial sampling state
  bool m_isSampling = true;
  QTimer* m_initalSampleTimer = nullptr;
  QTimer* m_mergeTimer = nullptr;
  int m_predictableTimerValue;
  QMutex m_directCallbacksMutex;
  std::vector<IDmxReceivedCallback*> m_dmxReceivedCallbacks;
  bool m_mergeAll = true; // A flag to initiate a complete remerge of everything
  unsigned int m_mergesPerSecond = 0;
  int m_mergeCounter = 0;
  QElapsedTimer m_mergesPerSecondTimer;

  sACNRxSocket::sBindStatus m_bindStatus;
};


#endif // SACNLISTENER_H
