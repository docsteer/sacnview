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

#include "sacnlistener.h"
#include "streamcommon.h"
#include "securesacn.h"
#include "preferences.h"
#include "defpack.h"
#include "preferences.h"
#include <QDebug>
#include <QThread>
#include <QPoint>
#include <math.h>
#include <QtGlobal>
#include "sacndiscovery.h"
#include "sacnsynchronization.h"

#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
#include <QNetworkDatagram>
#endif

//The amount of ms to wait before determining that a newly discovered source is not doing per-channel-priority
#define WAIT_PRIORITY 1500

//The time during which to sample
#define SAMPLE_TIME 1500

//Background merge interval
#define BACKGROUND_MERGE 500

sACNListener::sACNListener(int universe, QObject* parent)
  : QObject(parent)
  , m_merged_levels(DMX_SLOT_MAX, sACNMergedAddress())
  , m_universe(universe)
{
  qRegisterMetaType<QHostAddress>("QHostAddress");
}

sACNListener::~sACNListener()
{
  m_initalSampleTimer->deleteLater();
  m_mergeTimer->deleteLater();
  qDeleteAll(m_sockets);
  qDebug() << this << ": stopping";
}

void sACNListener::startReception()
{
  qDebug() << this << ": Starting universe" << m_universe;

  // Clear the levels array
  std::fill(std::begin(m_last_levels), std::end(m_last_levels), -1);

  if (Preferences::Instance().GetNetworkListenAll() && !Preferences::Instance().networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack)) {
    // Listen on ALL interfaces and not working offline
    for (const auto& interface : QNetworkInterface::allInterfaces())
    {
      // If the interface is ok for use...
      if (Preferences::Instance().interfaceSuitable(interface))
      {
        startInterface(interface);
      }
    }
  }
  else {
    // Listen only to selected interface
    startInterface(Preferences::Instance().networkInterface());
    if (!m_sockets.empty()) {
      m_bindStatus.multicast = (m_sockets.back()->multicastInterface().isValid()) ? sACNRxSocket::BIND_OK : sACNRxSocket::BIND_FAILED;
      m_bindStatus.unicast = (m_sockets.back()->state() == QAbstractSocket::BoundState) ? sACNRxSocket::BIND_OK : sACNRxSocket::BIND_FAILED;
    }
  }

  // Start intial sampling
  m_initalSampleTimer = new QTimer(this);
  m_initalSampleTimer->setSingleShot(true);
  m_initalSampleTimer->setInterval(SAMPLE_TIME);
  connect(m_initalSampleTimer, &QTimer::timeout, this, &sACNListener::sampleExpiration, Qt::DirectConnection);
  m_initalSampleTimer->start();

  // Merge is performed whenever a packet arrives and every BACKGROUND_MERGE interval
  m_elapsedTime.start();
  m_mergesPerSecondTimer.start();
  m_mergeTimer = new QTimer(this);
  m_mergeTimer->setInterval(BACKGROUND_MERGE);
  connect(m_mergeTimer, &QTimer::timeout, this, &sACNListener::performMerge, Qt::DirectConnection);
  connect(m_mergeTimer, &QTimer::timeout, this, &sACNListener::checkSourceExpiration, Qt::DirectConnection);
  m_mergeTimer->start();

  // Everything is set
  emit listenerStarted(m_universe);
}

void sACNListener::startInterface(const QNetworkInterface& iface)
{
  m_sockets.push_back(new sACNRxSocket(iface));
  sACNRxSocket::sBindStatus status = m_sockets.back()->bind(m_universe);
  if (status.unicast == sACNRxSocket::BIND_OK || status.multicast == sACNRxSocket::BIND_OK) {
    connect(m_sockets.back(), &QUdpSocket::readyRead, this, &sACNListener::readPendingDatagrams, Qt::DirectConnection);
  }
  else {
    // Failed to bind
    m_sockets.pop_back();
  }

  if ((m_bindStatus.unicast == sACNRxSocket::BIND_UNKNOWN) || (m_bindStatus.unicast == sACNRxSocket::BIND_OK))
    m_bindStatus.unicast = status.unicast;
  if ((m_bindStatus.multicast == sACNRxSocket::BIND_UNKNOWN) || (m_bindStatus.multicast == sACNRxSocket::BIND_OK))
    m_bindStatus.multicast = status.multicast;
}

void sACNListener::sampleExpiration()
{
  m_isSampling = false;
  qDebug() << this << ": Sampling has ended";
}

void sACNListener::checkSourceExpiration()
{
  char cidstr[CID::CIDSTRINGBYTES];
  for (std::vector<sACNSource*>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
  {
    if ((*it)->src_valid)
    {
      if ((*it)->active.Expired() && (*it)->priority_wait.Expired())
      {
        (*it)->src_valid = false;
        (*it)->src_stable = false;
        CID::CIDIntoString((*it)->src_cid, cidstr);
        emit sourceLost(*it);
        m_mergeAll = true;
        qDebug() << "Lost source " << cidstr;
      }
      else if ((*it)->doing_per_channel && (*it)->priority_wait.Expired())
      {
        CID::CIDIntoString((*it)->src_cid, cidstr);
        (*it)->doing_per_channel = false;
        emit sourceChanged(*it);
        m_mergeAll = true;
        qDebug() << this << ": Source stopped sending per-channel priority" << cidstr;
      }
    }
  }
}

void sACNListener::readPendingDatagrams()
{
#if (QT_VERSION == QT_VERSION_CHECK(5, 9, 3))
#error "QT5.9.3 QUdpSocket::readDatagram Returns incorrect infomation: https://bugreports.qt.io/browse/QTBUG-64784"
#endif
#if (QT_VERSION == QT_VERSION_CHECK(5, 10, 0))
#error "QT5.10.0 QUdpSocket::readDatagram Returns incorrect infomation: https://bugreports.qt.io/browse/QTBUG-65099"
#endif

  // Check all sockets
  for (auto m_socket : m_sockets)
  {
    while (m_socket->hasPendingDatagrams())
    {
      QNetworkDatagram datagram = m_socket->receiveDatagram();

      if (datagram.data().isEmpty())
        break;

      /* Localhost - Allowed
       * Relevant Multicast - Allowed
       * Unicast for this interface - Allowed (Universe checked later)
       * Broadcast - Rejected
       */
      if (datagram.destinationAddress().isBroadcast())
        break;

      QList<QHostAddress> interfaceAddress;
      for (const auto& address : m_socket->getBoundInterface().addressEntries())
        interfaceAddress << address.ip();

      if (
        // Relevant Multicast
        (datagram.destinationAddress().isMulticast() && datagram.destinationAddress() == m_socket->getMulticastAddr())
        ||
        // Unicast for this interface
        interfaceAddress.contains(QHostAddress(datagram.destinationAddress()))
        )
      {
        processDatagram(
          datagram.data(),
          datagram.destinationAddress(),
          datagram.senderAddress());
      }
    }
  }
}

void sACNListener::processDatagram(const QByteArray& data, const QHostAddress& destination, const QHostAddress& sender)
{
  if (QThread::currentThread() != this->thread())
  {
    QMetaObject::invokeMethod(
      this,
      "processDatagram",
      Q_ARG(QByteArray, data),
      Q_ARG(QHostAddress, destination),
      Q_ARG(QHostAddress, sender));
    return;
  };

  QMutexLocker locker(&m_processMutex);

  // Process packet
  quint32 root_vector = 0;
  CID source_cid;
  quint8 start_code = 0;
  quint8 sequence = 0;
  quint16 universe = 0;
  quint16 slot_count = 0;
  const quint8* pdata = nullptr;
  char source_name[SOURCE_NAME_SIZE] = {};
  quint8 priority = 0;
  /*
   * These only apply to the ratified version of the spec, so we will hardwire
   * them to defaults just in case they never get set.
  */
  quint16 synchronization = NOT_SYNCHRONIZED_VALUE; // E1.31:2018
  quint8 options = NO_OPTIONS_VALUE;

  const e_ValidateStreamHeader streamHeaderVersion = ValidateStreamHeader(reinterpret_cast<const quint8*>(data.data()), data.length(), root_vector, source_cid, source_name, priority,
    start_code, synchronization, sequence, options, universe, slot_count, pdata);

  switch (streamHeaderVersion)
  {
  case e_ValidateStreamHeader::StreamHeader_Invalid:
    // Recieved a packet but not valid. Log and discard
    qDebug() << this << ": Invalid Packet";
    return;

  case e_ValidateStreamHeader::StreamHeader_Draft:
  case e_ValidateStreamHeader::StreamHeader_Ratified:
    break;

  case e_ValidateStreamHeader::StreamHeader_Extended:
  {
    quint32 vector;
    if (static_cast<size_t>(data.length()) > ROOT_VECTOR_ADDR + sizeof(vector))
    {
      vector = UpackBUint32(reinterpret_cast<const quint8*>(data.data()) + FRAMING_VECTOR_ADDR);
      switch (vector)
      {
      case VECTOR_E131_EXTENDED_DISCOVERY:
        sACNDiscoveryRX::getInstance()->processPacket(reinterpret_cast<const quint8*>(data.data()), data.length());
        break;

      case VECTOR_E131_EXTENDED_SYNCHRONIZATION:
        sACNSynchronizationRX::getInstance()->processPacket(reinterpret_cast<const quint8*>(data.data()), data.length(), destination, sender);
        break;

      default:
        qDebug() << this << ": Unknown Extended Packet";
      }
    }
  } return;

  case e_ValidateStreamHeader::StreamHeader_Pathway_Secure:
    if (!Preferences::Instance().GetPathwaySecureRx())
    {
      qDebug() << this << ": Ignore Pathway secure";
      return;
    }
    break;

  default:
  case e_ValidateStreamHeader::StreamHeader_Unknown:
    qDebug() << this << ": Unkown Root Vector";
    return;
  }

  // Packet is now know to contain DMX Level Data

  // Wrong universe
  if (m_universe != universe)
  {
    // Was it unicast? Send to correct listener (if listening)
    if (!destination.isMulticast() && !destination.isBroadcast())
    {
      // Unicast, send to correct listener!
      decltype(sACNManager::Instance().getListenerList()) listenerList
        = sACNManager::Instance().getListenerList();
      auto it = listenerList.find(universe);
      if (it != listenerList.end())
        it.value().toStrongRef()->processDatagram(data, destination, sender);
    }
    else {
      // Log and discard
      qDebug() << this << ": Rejecting universe" << universe << "sent to" << destination;
    }
    return;
  }

  // Listen to preview?
  const bool preview = (PREVIEW_DATA_OPTION == (options & PREVIEW_DATA_OPTION));
  if (preview && !Preferences::Instance().GetBlindVisualizer())
  {
    qDebug() << this << ": Ignore preview";
    return;
  }

  sACNSource* ps = nullptr; // Pointer to the source
  bool newsourcenotify = false;
  bool validpacket = true;  //whether or not we will actually process the packet

  // Find existing known source
  auto it = std::find_if(m_sources.begin(), m_sources.end(), [source_cid](sACNSource* source){ return source && source->src_cid == source_cid; });
  if (it != m_sources.end())
  {
    ps = (*it);

    // Verify Pathway Secure DMX security features, do this before updating the active flag
    if (streamHeaderVersion == e_ValidateStreamHeader::StreamHeader_Pathway_Secure) {
      PathwaySecure::VerifyStreamSecurity(reinterpret_cast<const quint8*>(data.data()), data.size(),
        Preferences::Instance().GetPathwaySecureRxPassword(), *ps);
    }
    else {
      ps->pathway_secure.passwordOk = false;
      ps->pathway_secure.sequenceOk = false;
      ps->pathway_secure.digestOk = false;
    }

    if (!ps->src_valid)
    {
      // This is a source which is coming back online, so we need to repeat the steps
      // for initial source aquisition
      ps->active.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
      ps->lastseq = sequence;
      ps->src_valid = true;
      ps->doing_dmx = (start_code == STARTCODE_DMX);
      ps->doing_per_channel = ps->waited_for_dd = false;
      newsourcenotify = false;
      ps->priority_wait.SetInterval(std::chrono::milliseconds(WAIT_PRIORITY));
    }

    if ((options & STREAM_TERMINATED_OPTION) == STREAM_TERMINATED_OPTION)
    {
      // Source is terminating

      //by setting this flag to false, 0xdd packets that may come in while the terminated data
      //packets come in won't reset the priority_wait timer
      ps->waited_for_dd = false;
      if (start_code == STARTCODE_DMX)
        ps->doing_dmx = false;

      //"Upon receipt of a packet containing this bit set
      //to a value of 1, a receiver shall enter network
      //data loss condition.  Any property values in
      //these packets shall be ignored"
      ps->active.SetInterval(std::chrono::milliseconds(m_ssHLL));  //We factor in the hold last look time here, rather than 0

      if (ps->doing_per_channel)
        ps->priority_wait.SetInterval(std::chrono::milliseconds(m_ssHLL)); //We factor in the hold last look time here, rather than 0

      validpacket = false;
    }
    else
    {
      //Based on the start code, update the timers
      switch (start_code)
      {
      case STARTCODE_DMX:
      {
        //No matter how valid, we got something -- but we'll tweak the interval for any hll change
        ps->doing_dmx = true;
        ps->active.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
      } break;

      case STARTCODE_PRIORITY: if (ps->waited_for_dd)
      {
        ps->doing_per_channel = true;  //The source could have stopped sending dd for a while.
        ps->priority_wait.Reset();
      } break;
      }

      //Validate the sequence number, updating the stored one
      //The two's complement math is to handle rollover, and we're explicitly
      //doing assignment to force the type sizes.  A negative number means
      //we got an "old" one, but we assume that anything really old is possibly
      //due the device having rebooted and starting the sequence over.
      const qint16 result = reinterpret_cast<qint8&>(sequence) - reinterpret_cast<qint8&>(ps->lastseq);
      if (result != 1)
      {
        ps->jumps++;
        if ((result <= 0) && (result > -20))
        {
          validpacket = false;
          ps->seqErr++;
        }
      }

      ps->lastseq = sequence;

      //This next bit is a little tricky.  We want to wait for dd packets (sampling period
      //tweaks aside) and notify them with the dd packet first, but we don't want to do that
      //if we've never seen a dmx packet from the source.
      if (!ps->doing_dmx)
      {
        /* For fault finding an installation which only sends 0xdd for a universe
         * (For example: ETC Cobalt does this for, currenlty, unpatched universes with in it's network map)
         * We do want to say that having not sent dimmer data is a valid source/packet
         */
         // validpacket = false;
        ps->priority_wait.Reset();  //We don't want to let the priority timer run out
      }
      else if (!ps->waited_for_dd && validpacket)
      {
        if (start_code == STARTCODE_PRIORITY)
        {
          ps->waited_for_dd = true;
          ps->doing_per_channel = true;
          ps->priority_wait.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
          newsourcenotify = true;
        }
        else if (ps->priority_wait.Expired())
        {
          ps->waited_for_dd = true;
          ps->doing_per_channel = false;
          ps->priority_wait.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));  //In case the source later decides to sent 0xdd packets
          newsourcenotify = true;
        }
        else
          newsourcenotify = validpacket = false;
      }
    }
    // Found the source, and we're ready to process the packet
  }


  if (!validpacket)
  {
    qDebug() << this << ": Source coming up, not processing packet";
    return;
  }

  if (ps == nullptr)  // Add a new source to the list
  {
    ps = new sACNSource(source_cid, universe);

    m_sources.push_back(ps);

    ps->name = QString::fromUtf8(source_name);
    ps->ip = sender;
    ps->synchronization = synchronization;
    ps->active.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
    ps->lastseq = sequence;
    ps->src_valid = true;
    ps->src_stable = true;
    ps->doing_dmx = (start_code == STARTCODE_DMX);
    //If we are in the sampling period, let all packets through
    if (m_isSampling)
    {
      ps->waited_for_dd = true;
      ps->doing_per_channel = (start_code == STARTCODE_PRIORITY);
      newsourcenotify = true;
      ps->priority_wait.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
    }
    else
    {
      //If we aren't sampling, we want the earlier logic to set the state
      ps->doing_per_channel = ps->waited_for_dd = false;
      newsourcenotify = false;
      ps->priority_wait.SetInterval(std::chrono::milliseconds(WAIT_PRIORITY));
    }

    validpacket = newsourcenotify;


    // This is a brand new source
    qDebug() << this << ": Found new source name " << source_name;
    m_mergeAll = true;
    emit sourceFound(ps);
  }

  if (newsourcenotify)
  {
    // This is a source that came back online
    qDebug() << this << ": Source came back name " << source_name;
    m_mergeAll = true;
    emit sourceResumed(ps);
    emit sourceChanged(ps);
  }


  //Finally, Process the buffer
  if (validpacket)
  {
    ps->source_params_change = false;

    QString name = QString::fromUtf8(source_name);

    if (ps->ip != sender)
    {
      ps->ip = sender;
      ps->source_params_change = true;
    }

    StreamingACNProtocolVersion protocolVersion = sACNProtocolUnknown;
    switch (root_vector) {
    case VECTOR_ROOT_E131_DATA:
      protocolVersion = sACNProtocolRelease;
      break;

    case VECTOR_ROOT_E131_DATA_DRAFT:
      protocolVersion = sACNProtocolDraft;
      break;

    case VECTOR_ROOT_E131_DATA_PATHWAY_SECURE:
      protocolVersion = sACNProtocolPathwaySecure;
      break;

    default:
      protocolVersion = sACNProtocolUnknown;
      break;
    }

    if (ps->protocol_version != protocolVersion)
    {
      ps->protocol_version = protocolVersion;
      ps->source_params_change = true;
    }

    if (start_code == STARTCODE_DMX)
    {
      if (ps->name != name)
      {
        ps->name = name;
        ps->source_params_change = true;
      }
      if (ps->isPreview != preview)
      {
        ps->isPreview = preview;
        ps->source_params_change = true;
      }
      if (ps->priority != priority)
      {
        ps->priority = priority;
        ps->source_params_change = true;
      }
      if (ps->synchronization != synchronization)
      {
        ps->synchronization = synchronization;
        ps->source_params_change = true;
      }

      ps->storeReceivedLevels(pdata, slot_count);

      // FPS Counter - we count only DMX frames
      ps->fpscounter.newFrame();
      if (ps->fpscounter.isNewFPS())
        ps->source_params_change = true;
    }
    else if (start_code == STARTCODE_PRIORITY)
    {
      if (!Preferences::Instance().GetETCDD())
      { // DD is disabled, fill with universe priority.
        // TODO: Does this actually ever do anything?
        std::array<uint8_t, DMX_SLOT_MAX> univ_priority;
        univ_priority.fill(ps->priority);
        ps->storeReceivedPriorities(univ_priority.data(), slot_count);
      }
      else
      {
        ps->storeReceivedPriorities(pdata, slot_count);
      }
    }

    if (ps->source_params_change)
    {
      emit sourceChanged(ps);
      ps->source_params_change = false;
    }

    // Listen to synchronization source
    if (ps->synchronization &&
      (!ps->sync_listener || ps->sync_listener->universe() != ps->synchronization)) {
      ps->sync_listener = sACNManager::Instance().getListener(ps->synchronization);
    }

    // Merge
    performMerge();
  }
}

inline bool isPatched(const sACNSource& source, uint16_t address)
{
  // Can only be unpatched if we have DD packets
  if (!source.doing_per_channel)
    return true;

  // Priority not 0
  return source.priority_array[address] != 0;
}

void sACNListener::performMerge()
{
  //array of addresses to merge. to prevent duplicates and because you can have
  //an odd collection of addresses, addresses[n] would be 'n' for the value in question
  // and -1 if not required
  int addresses_to_merge[DMX_SLOT_MAX];
  int number_of_addresses_to_merge = 0;

  memset(addresses_to_merge, -1, sizeof(int) * DMX_SLOT_MAX);

  {
    QMutexLocker locker(&m_monitoredChannelsMutex);
    for (auto const& chan : qAsConst(m_monitoredChannels))
    {
      QPointF data;
      data.setX(m_elapsedTime.nsecsElapsed() / 1000000.0);
      data.setY(mergedLevels().at(chan).level);
      emit dataReady(chan, data);
    }
  }

  if (m_mergesPerSecondTimer.hasExpired(1000))
  {
    m_mergesPerSecond = m_mergeCounter;
    m_mergeCounter = 0;
    m_mergesPerSecondTimer.restart();
  }

  m_mergeCounter++;

  // Step one : find any addresses which have changed
  if (m_mergeAll) // Act like all addresses changed
  {
    number_of_addresses_to_merge = DMX_SLOT_MAX;
    for (int i = 0; i < DMX_SLOT_MAX; i++)
    {
      addresses_to_merge[i] = i;
    }

    m_mergeAll = false;
  }
  else
  {
    for (std::vector<sACNSource*>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
    {
      sACNSource* ps = *it;
      if (!ps->src_valid)
        continue; // Inactive source, ignore it
      if (!ps->source_levels_change)
        continue; // We don't need to consider this one, no change
      for (int i = 0; i < DMX_SLOT_MAX; i++)
      {
        if (ps->dirty_array[i])
        {
          addresses_to_merge[i] = i;
          number_of_addresses_to_merge++;
        }
      }
      // Clear the flags
      ps->dirty_array.fill(false);
      ps->source_levels_change = false;
    }
  }

  if (number_of_addresses_to_merge == 0) return; // Nothing to do

  // Clear out the sources list for all the affected channels, we'll be refreshing it

  int skipCounter = 0;
  for (int i = 0; i < DMX_SLOT_MAX && i < (number_of_addresses_to_merge + skipCounter); i++)
  {
    QMutexLocker mergeLocker(&m_merged_levelsMutex);
    m_merged_levels[i].changedSinceLastMerge = false;
    if (addresses_to_merge[i] == -1) {
      ++skipCounter;
      continue;
    }
    sACNMergedAddress* pAddr = &m_merged_levels[addresses_to_merge[i]];
    pAddr->otherSources.clear();
  }

  // Find the highest priority source for each address we need to work on

  int levels[DMX_SLOT_MAX];
  memset(&levels, -1, sizeof(levels));

  int priorities[DMX_SLOT_MAX];
  memset(&priorities, -1, sizeof(priorities));

  QMultiMap<int, sACNSource*> addressToSourceMap;

  // Find the highest priority for the address
  bool secureDataOnly = false;
  if (Preferences::Instance().GetPathwaySecureRx())
    secureDataOnly = Preferences::Instance().GetPathwaySecureRxDataOnly();
  for (std::vector<sACNSource*>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
  {
    sACNSource* ps = *it;

    if (ps->src_valid && !ps->active.Expired() && !ps->doing_per_channel)
    {
      // Set the priority array for sources which are not doing per-channel
      ps->priority_array.fill(ps->priority);
    }

    skipCounter = 0;
    for (int i = 0; i < DMX_SLOT_MAX && i < (number_of_addresses_to_merge + skipCounter); i++)
    {
      QMutexLocker mergeLocker(&m_merged_levelsMutex);
      if (addresses_to_merge[i] == -1) {
        ++skipCounter;
        continue;
      }
      int address = addresses_to_merge[i];

      if (
        ps->src_valid // Valid Source
        && !ps->active.Expired() // Not expired
        && !(ps->priority_array[address] < priorities[address]) // Not lesser priority
        && isPatched(*ps, address) // Priority > 0 if DD
        && (address < ps->slot_count) // Sending the required slot
        && ((!secureDataOnly) || (secureDataOnly && ps->pathway_secure.isSecure())) // Is secure, if only displaying secure sources
        )
      {
        if (ps->priority_array[address] > priorities[address])
        {
          // Source of higher priority
          priorities[address] = ps->priority_array[address];
          addressToSourceMap.remove(address);
        }
        addressToSourceMap.insert(address, ps);
      }

      if (
        ps->src_valid // Valid Source
        && !ps->active.Expired() // Not Expired
        && isPatched(*ps, address) // Priority > 0 if DD
        && (address < ps->slot_count) // Sending the required slot
        )
        m_merged_levels[addresses_to_merge[i]].otherSources.insert(ps);
    }
  }

  // Next, find highest level for the highest prioritized sources
  skipCounter = 0;
  for (int i = 0; i < DMX_SLOT_MAX && i < (number_of_addresses_to_merge + skipCounter); i++)
  {
    QMutexLocker mergeLocker(&m_merged_levelsMutex);
    if (addresses_to_merge[i] == -1) {
      ++skipCounter;
      continue;
    }
    int address = addresses_to_merge[i];
    QList<sACNSource*> sourceList = addressToSourceMap.values(address);

    if (sourceList.count() == 0)
    {
      m_merged_levels[address].level = -1;
      m_merged_levels[address].winningSource = NULL;
      m_merged_levels[address].otherSources.clear();
      m_merged_levels[address].winningPriority = priorities[address];
    }
    for (auto s : sourceList)
    {
      if (s->level_array[address] > levels[address])
      {
        levels[address] = s->level_array[address];
        m_merged_levels[address].changedSinceLastMerge = (m_merged_levels[address].level != levels[address]);
        m_merged_levels[address].level = levels[address];
        m_merged_levels[address].winningSource = s;
        m_merged_levels[address].winningPriority = priorities[address];
      }
    }
    // Remove the winning source from the list of others
    if (m_merged_levels[address].winningSource)
      m_merged_levels[address].otherSources.remove(m_merged_levels[address].winningSource);
  }

  // Tell people..
  emit levelsChanged();
}

