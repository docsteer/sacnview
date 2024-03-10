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

sACNListener::sACNListener(int universe, QObject *parent)
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
    qDebug() << "sACNListener" << QThread::currentThreadId() << ": stopping";
}

void sACNListener::startReception()
{
    qDebug() << "sACNListener" << QThread::currentThreadId() << ": Starting universe" << m_universe;

    // Clear the levels array
    std::fill(std::begin(m_last_levels), std::end(m_last_levels), -1);

    if (Preferences::Instance().GetNetworkListenAll() && !Preferences::Instance().networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack)) {
        // Listen on ALL interfaces and not working offline
        for (const auto &interface : QNetworkInterface::allInterfaces())
        {
            // If the interface is ok for use...
            if(Preferences::Instance().interfaceSuitable(interface))
            {
                startInterface(interface);
            }
        }
    } else {
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

void sACNListener::startInterface(const QNetworkInterface &iface)
{
    m_sockets.push_back(new sACNRxSocket(iface));
    sACNRxSocket::sBindStatus status = m_sockets.back()->bind(m_universe);
    if (status.unicast == sACNRxSocket::BIND_OK || status.multicast == sACNRxSocket::BIND_OK) {
        connect(m_sockets.back(), &QUdpSocket::readyRead, this, &sACNListener::readPendingDatagrams, Qt::DirectConnection);
    } else {
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
    qDebug() << "sACNListener" << QThread::currentThreadId() << ": Sampling has ended";
}

void sACNListener::checkSourceExpiration()
{
    char cidstr [CID::CIDSTRINGBYTES];
    for(std::vector<sACNSource *>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
    {
        if((*it)->src_valid)
        {
            if((*it)->active.Expired() && (*it)->priority_wait.Expired())
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
                qDebug() << "sACNListener" << QThread::currentThreadId() << ": Source stopped sending per-channel priority" << cidstr;
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
        while(m_socket->hasPendingDatagrams())
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
            for (const auto &address : m_socket->getBoundInterface().addressEntries())
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

void sACNListener::processDatagram(const QByteArray &data, const QHostAddress &destination, const QHostAddress &sender)
{
    if(QThread::currentThread()!=this->thread())
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
    quint32 root_vector;
    CID source_cid;
    quint8 start_code;
    quint8 sequence;
    quint16 universe;
    quint16 slot_count;
    quint8* pdata;
    char source_name [SOURCE_NAME_SIZE];
    quint8 priority;
    /*
     * These only apply to the ratified version of the spec, so we will hardwire
     * them to be 0 just in case they never get set.
    */
    quint16 synchronization = NOT_SYNCHRONIZED_VALUE;
    quint8 options = NO_OPTIONS_VALUE;
    bool preview = false;

    switch (ValidateStreamHeader((quint8*)data.data(), data.length(), root_vector, source_cid, source_name, priority,
            start_code, synchronization, sequence, options, universe, slot_count, pdata))
    {
    case e_ValidateStreamHeader::StreamHeader_Invalid:
        // Recieved a packet but not valid. Log and discard
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Invalid Packet";
        return;

    case e_ValidateStreamHeader::StreamHeader_Unknown:
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Unkown Root Vector";
        return;

    case e_ValidateStreamHeader::StreamHeader_Extended:
        quint32 vector;
        if (static_cast<size_t>(data.length()) > ROOT_VECTOR_ADDR + sizeof(vector))
        {
            vector = UpackBUint32((quint8*)data.data() + FRAMING_VECTOR_ADDR);
            switch (vector)
            {
            case VECTOR_E131_EXTENDED_DISCOVERY:
                sACNDiscoveryRX::getInstance()->processPacket((quint8*)data.data(), data.length());
                break;

            case VECTOR_E131_EXTENDED_SYNCHRONIZATION:
                sACNSynchronizationRX::getInstance()->processPacket((quint8*)data.data(), data.length(), destination, sender);
                break;

            default:
                qDebug() << "sACNListener" << QThread::currentThreadId() << ": Unknown Extended Packet";
            }
        }
        return;

        case e_ValidateStreamHeader::StreamHeader_Pathway_Secure:
            if (!Preferences::Instance().GetPathwaySecureRx())
            {
                qDebug() << "sACNListener" << QThread::currentThreadId() << ": Ignore Pathway secure";
                return;
            }

    default:
        break;
    }

    // Wrong universe
    if(m_universe != universe)
    {
        // Was it unicast? Send to correct listener (if listening)
        if (!destination.isMulticast() && !destination.isBroadcast())
        {
            // Unicast, send to correct listener!
            decltype(sACNManager::Instance().getListenerList()) listenerList
                    = sACNManager::Instance().getListenerList();
            if (listenerList.contains(universe))
                listenerList[universe].toStrongRef()->processDatagram(data, destination, sender);
            return;
        } else {
            // Log and discard
            qDebug() << "sACNListener" << QThread::currentThreadId()
                     << ": Rejecting universe" << universe << "sent to" << destination;
            return;
        }
    }

    // Listen to preview?
    preview = (PREVIEW_DATA_OPTION == (options & PREVIEW_DATA_OPTION));
    if ((preview) && !Preferences::Instance().GetBlindVisualizer())
    {
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Ignore preview";
        return;
    }

    sACNSource *ps = NULL; // Pointer to the source
    bool foundsource = false;
    bool newsourcenotify = false;
    bool validpacket = true;  //whether or not we will actually process the packet

    for(std::vector<sACNSource *>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
    {
        if((*it)->src_cid == source_cid)
        {
            foundsource = true;
            ps = *it;

            // Verify Pathway Secure DMX security features, do this before updating the active flag
            if (root_vector == VECTOR_ROOT_E131_DATA_PATHWAY_SECURE) {
                PathwaySecure::VerifyStreamSecurity(
                            (quint8*)data.data(), data.size(),
                            Preferences::Instance().GetPathwaySecureRxPassword(),
                            *ps);
            } else {
                ps->pathway_secure.passwordOk = false;
                ps->pathway_secure.sequenceOk = false;
                ps->pathway_secure.digestOk = false;
            }

            if(!ps->src_valid)
            {
                // This is a source which is coming back online, so we need to repeat the steps
                // for initial source aquisition
                ps->active.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
                ps->lastseq = sequence;
                ps->src_cid = source_cid;
                ps->src_valid = true;
                ps->doing_dmx = (start_code == STARTCODE_DMX);
                ps->doing_per_channel = ps->waited_for_dd = false;
                newsourcenotify = false;
                ps->priority_wait.SetInterval(std::chrono::milliseconds(WAIT_PRIORITY));
            }

            if(
                ((root_vector == VECTOR_ROOT_E131_DATA) || root_vector == VECTOR_ROOT_E131_DATA_PATHWAY_SECURE)
                && ((options & STREAM_TERMINATED_OPTION) == STREAM_TERMINATED_OPTION))
            {
              //by setting this flag to false, 0xdd packets that may come in while the terminated data
              //packets come in won't reset the priority_wait timer
              (*it)->waited_for_dd = false;
              if(start_code == STARTCODE_DMX)
                (*it)->doing_dmx = false;

              //"Upon receipt of a packet containing this bit set
              //to a value of 1, a receiver shall enter network
              //data loss condition.  Any property values in
              //these packets shall be ignored"
              (*it)->active.SetInterval(std::chrono::milliseconds(m_ssHLL));  //We factor in the hold last look time here, rather than 0

              if((*it)->doing_per_channel)
                  (*it)->priority_wait.SetInterval(std::chrono::milliseconds(m_ssHLL)); //We factor in the hold last look time here, rather than 0

              validpacket = false;
              break;
            }

            //Based on the start code, update the timers
            if(start_code == STARTCODE_DMX)
            {
                //No matter how valid, we got something -- but we'll tweak the interval for any hll change
                (*it)->doing_dmx = true;
                (*it)->active.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
            }
            else if(start_code == STARTCODE_PRIORITY && (*it)->waited_for_dd)
            {
                (*it)->doing_per_channel = true;  //The source could have stopped sending dd for a while.
                (*it)->priority_wait.Reset();
            }

            //Validate the sequence number, updating the stored one
            //The two's complement math is to handle rollover, and we're explicitly
            //doing assignment to force the type sizes.  A negative number means
            //we got an "old" one, but we assume that anything really old is possibly
            //due the device having rebooted and starting the sequence over.
            qint8 result = ((qint8)sequence) - ((qint8)((*it)->lastseq));
            if(result!=1)
                (*it)->jumps++;
            if((result <= 0) && (result > -20))
            {
                validpacket = false;
                (*it)->seqErr++;
            }
            else
                (*it)->lastseq = sequence;

            //This next bit is a little tricky.  We want to wait for dd packets (sampling period
            //tweaks aside) and notify them with the dd packet first, but we don't want to do that
            //if we've never seen a dmx packet from the source.
            if(!(*it)->doing_dmx)
            {
                /* For fault finding an installation which only sends 0xdd for a universe
                 * (For example: ETC Cobalt does this for, currenlty, unpatched universes with in it's network map)
                 * We do want to say that having not sent dimmer data is a valid source/packet
                 */
                // validpacket = false;
                (*it)->priority_wait.Reset();  //We don't want to let the priority timer run out
            }
            else if(!(*it)->waited_for_dd && validpacket)
            {
                if(start_code == STARTCODE_PRIORITY)
                {
                    (*it)->waited_for_dd = true;
                    (*it)->doing_per_channel = true;
                    (*it)->priority_wait.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
                    newsourcenotify = true;
                }
                else if((*it)->priority_wait.Expired())
                {
                    (*it)->waited_for_dd = true;
                    (*it)->doing_per_channel = false;
                    (*it)->priority_wait.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));  //In case the source later decides to sent 0xdd packets
                    newsourcenotify = true;
                }
                else
                    newsourcenotify = validpacket = false;
            }

        //Found the source, and we're ready to process the packet
        }
    }

    if(!validpacket)
    {
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Source coming up, not processing packet";
        return;
    }

    if(!foundsource)  //Add a new source to the list
    {
        ps = new sACNSource();

        m_sources.push_back(ps);

        ps->name = QString::fromUtf8(source_name);
        ps->ip = sender;
        ps->universe = universe;
        ps->synchronization = synchronization;
        ps->active.SetInterval(std::chrono::milliseconds(E131_NETWORK_DATA_LOSS_TIMEOUT + m_ssHLL));
        ps->lastseq = sequence;
        ps->src_cid = source_cid;
        ps->src_valid = true;
        ps->src_stable = true;
        ps->doing_dmx = (start_code == STARTCODE_DMX);
        //If we are in the sampling period, let all packets through
        if(m_isSampling)
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
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Found new source name " << source_name;
        m_mergeAll = true;
        emit sourceFound(ps);
    }

    if (newsourcenotify)
    {
        // This is a source that came back online
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Source came back name " << source_name;
        m_mergeAll = true;
        emit sourceResumed(ps);
        emit sourceChanged(ps);
    }


    //Finally, Process the buffer
    if(validpacket)
    {
        ps->source_params_change = false;

        QString name = QString::fromUtf8(source_name);

        if(ps->ip != sender)
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

        if(ps->protocol_version!=protocolVersion)
        {
            ps->protocol_version = protocolVersion;
            ps->source_params_change = true;
        }

        if(start_code == STARTCODE_DMX)
        {
            if(ps->name!=name)
            {
                ps->name = name;
                ps->source_params_change = true;
            }
            if(ps->isPreview != preview)
            {
                ps->isPreview = preview;
                ps->source_params_change = true;
            }
            if(ps->priority != priority)
            {
                ps->priority = priority;
                ps->source_params_change = true;
            }
            if(ps->synchronization != synchronization)
            {
                ps->synchronization = synchronization;
                ps->source_params_change = true;
            }
            // This is DMX
            // Copy the last array back
            memcpy(ps->last_level_array, ps->level_array, DMX_SLOT_MAX);
            // Fill in the new array
            memset(ps->level_array, 0, DMX_SLOT_MAX);
            memcpy(ps->level_array, pdata, slot_count);

            // Slot count change, re-merge all slots
            if(ps->slot_count != slot_count)
            {
                ps->slot_count = slot_count;
                ps->source_params_change = true;
                ps->source_levels_change = true;
                for(int i=0; i<DMX_SLOT_MAX; i++)
                    ps->dirty_array[i] |= true;
            }

            // Compare the two
            for(int i=0; i<DMX_SLOT_MAX; i++)
            {
                if(ps->level_array[i]!=ps->last_level_array[i])
                {
                    ps->dirty_array[i] |= true;
                    ps->source_levels_change = true;
                }
            }

            // FPS Counter - we count only DMX frames
            ps->fpscounter.newFrame();
            if (ps->fpscounter.isNewFPS())
                ps->source_params_change = true;
        }
        else if(start_code == STARTCODE_PRIORITY)
        {
            // Copy the last array back
            memcpy(ps->last_priority_array, ps->priority_array, DMX_SLOT_MAX);
            // Fill in the new array
            memset(ps->priority_array, 0, DMX_SLOT_MAX);
            if (!Preferences::Instance().GetETCDD())
            { // DD is disabled, fill with universe priority
                std::fill(std::begin(ps->priority_array), std::end(ps->priority_array), ps->priority);
            } else {
                memcpy(ps->priority_array, pdata, slot_count);
            }
            // Compare the two
            for(int i=0; i<DMX_SLOT_MAX; i++)
            {
                if(ps->priority_array[i]!=ps->last_priority_array[i])
                {
                    ps->dirty_array[i] |= true;
                    ps->source_levels_change = true;
                }
            }
        }

        if(ps->source_params_change)
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

inline bool isPatched(const sACNSource &source, uint16_t address)
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
        for(auto const &chan: qAsConst(m_monitoredChannels))
        {
            QPointF data;
            data.setX(m_elapsedTime.nsecsElapsed()/1000000.0);
            data.setY(mergedLevels().at(chan).level);
            emit dataReady(chan, data);
        }
    }

    if(m_mergesPerSecondTimer.hasExpired(1000))
    {
        m_mergesPerSecond = m_mergeCounter;
        m_mergeCounter = 0;
        m_mergesPerSecondTimer.restart();
    }

    m_mergeCounter++;

    // Step one : find any addresses which have changed
    if(m_mergeAll) // Act like all addresses changed
    {
        number_of_addresses_to_merge = DMX_SLOT_MAX;
        for(int i=0; i<DMX_SLOT_MAX; i++)
        {
            addresses_to_merge[i] = i;
        }

        m_mergeAll = false;
    }
    else
    {
        for(std::vector<sACNSource *>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
        {
            sACNSource *ps = *it;
            if(!ps->src_valid)
                continue; // Inactive source, ignore it
            if(!ps->source_levels_change)
                continue; // We don't need to consider this one, no change
            for(int i=0; i<DMX_SLOT_MAX; i++)
            {
                if(ps->dirty_array[i])
                {
                    addresses_to_merge[i] = i;
                    number_of_addresses_to_merge++;
                }
            }
            // Clear the flags
            memset(ps->dirty_array, 0 , DMX_SLOT_MAX);
            ps->source_levels_change = false;
        }
    }

    if(number_of_addresses_to_merge == 0) return; // Nothing to do

    // Clear out the sources list for all the affected channels, we'll be refreshing it

    int skipCounter = 0;
    for(int i=0; i < DMX_SLOT_MAX && i<(number_of_addresses_to_merge + skipCounter); i++)
    {
        QMutexLocker mergeLocker(&m_merged_levelsMutex);
        m_merged_levels[i].changedSinceLastMerge = false;
        if(addresses_to_merge[i] == -1) {
            ++skipCounter;
            continue;
        }
        sACNMergedAddress *pAddr = &m_merged_levels[addresses_to_merge[i]];
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
    for(std::vector<sACNSource *>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
    {
        sACNSource *ps = *it;
 
        if(ps->src_valid && !ps->active.Expired() && !ps->doing_per_channel)
        {
            // Set the priority array for sources which are not doing per-channel
            memset(ps->priority_array, ps->priority, sizeof(ps->priority_array));
        }

        skipCounter = 0;
        for(int i=0; i < DMX_SLOT_MAX && i<(number_of_addresses_to_merge + skipCounter); i++)
        {
            QMutexLocker mergeLocker(&m_merged_levelsMutex);
            if(addresses_to_merge[i] == -1) {
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

            if(
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
    for(int i=0; i < DMX_SLOT_MAX && i<(number_of_addresses_to_merge + skipCounter); i++)
    {
        QMutexLocker mergeLocker(&m_merged_levelsMutex);
        if(addresses_to_merge[i] == -1) {
            ++skipCounter;
            continue;
        }
        int address = addresses_to_merge[i];
        QList<sACNSource*> sourceList = addressToSourceMap.values(address);

        if(sourceList.count() == 0)
        {
            m_merged_levels[address].level = -1;
            m_merged_levels[address].winningSource = NULL;
            m_merged_levels[address].otherSources.clear();
            m_merged_levels[address].winningPriority = priorities[address];
        }
        for (auto s : sourceList)
        {
            if(s->level_array[address] > levels[address])
            {
                levels[address] = s->level_array[address];
                m_merged_levels[address].changedSinceLastMerge = (m_merged_levels[address].level != levels[address]);
                m_merged_levels[address].level = levels[address];
                m_merged_levels[address].winningSource = s;
                m_merged_levels[address].winningPriority = priorities[address];
            }
        }
        // Remove the winning source from the list of others
        if(m_merged_levels[address].winningSource)
            m_merged_levels[address].otherSources.remove(m_merged_levels[address].winningSource);
    }

    // Tell people..
    emit levelsChanged();
}

