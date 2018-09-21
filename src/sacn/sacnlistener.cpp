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

#include "sacnlistener.h"
#include "streamcommon.h"
#include "preferences.h"
#include "defpack.h"
#include "preferences.h"
#include <QDebug>
#include <QThread>
#include <QPoint>


//The amount of ms to wait before a source is considered offline or
//has stopped sending per-channel-priority packets
#define WAIT_OFFLINE 2500

//The amount of ms to wait before determining that a newly discovered source is not doing per-channel-priority
#define WAIT_PRIORITY 1500

//The time during which to sample
#define SAMPLE_TIME 1500

//Background merge interval
#define BACKGROUND_MERGE 500

sACNListener::sACNListener(int universe, QObject *parent) : QObject(parent),
    m_universe(universe),
    m_ssHLL(1000),
    m_isSampling(true),
    m_mergesPerSecond(0)
{
    m_merged_levels.reserve(DMX_SLOT_MAX);
    for(int i=0; i<DMX_SLOT_MAX; i++)
        m_merged_levels << sACNMergedAddress();
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
    memset(&m_last_levels, -1, sizeof(m_last_levels)/sizeof(m_last_levels[0]));

    if (Preferences::getInstance()->GetNetworkListenAll()) {
        // Listen on ALL interfaces
        for (auto interface : QNetworkInterface::allInterfaces())
        {
            // If the interface is ok for use...
            if(Preferences::getInstance()->interfaceSuitable(&interface))
            {
                startInterface(interface);
            }
        }

    } else {
        // Listen only to selected interface
        startInterface(Preferences::getInstance()->networkInterface());
    }

    // Start intial sampling
    m_initalSampleTimer = new QTimer(this);
    m_initalSampleTimer->setSingleShot(true);
    m_initalSampleTimer->setInterval(SAMPLE_TIME);
    connect(m_initalSampleTimer, SIGNAL(timeout()), this, SLOT(sampleExpiration()), Qt::DirectConnection);
    m_initalSampleTimer->start();

    // Merge is performed whenever a packet arrives and every BACKGROUND_MERGE interval
    m_elapsedTime.start();
    m_mergesPerSecondTimer.start();
    m_mergeTimer = new QTimer(this);
    m_mergeTimer->setInterval(BACKGROUND_MERGE);
    connect(m_mergeTimer, SIGNAL(timeout()), this, SLOT(performMerge()), Qt::DirectConnection);
    connect(m_mergeTimer, SIGNAL(timeout()), this, SLOT(checkSourceExpiration()), Qt::DirectConnection);
    m_mergeTimer->start();

    // Everything is set
    emit listenerStarted(m_universe);
}

void sACNListener::startInterface(QNetworkInterface iface)
{
    // Listen multicast
    m_sockets.push_back(new sACNRxSocket(iface));
    if (m_sockets.back()->bindMulticast(m_universe)) {
        connect(m_sockets.back(), SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()), Qt::DirectConnection);
        m_bindStatus.multicast = eBindStatus::BIND_OK;
    } else {
       // Failed to bind,
       m_sockets.pop_back();
       m_bindStatus.multicast = eBindStatus::BIND_FAILED;
    }

    // Listen unicast
    m_sockets.push_back(new sACNRxSocket(iface));
    if (m_sockets.back()->bindUnicast()) {
        connect(m_sockets.back(), SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()), Qt::DirectConnection);
        m_bindStatus.unicast = eBindStatus::BIND_OK;
    } else {
       // Failed to bind
       m_sockets.pop_back();
       m_bindStatus.unicast = eBindStatus::BIND_FAILED;
    }
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
            QByteArray data;
            data.resize(m_socket->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            m_socket->readDatagram(data.data(), data.size(),
                                    &sender, &senderPort);

            processDatagram(
                        data,
                        m_socket->localAddress(),
                        sender);
        }
    }
}

void sACNListener::processDatagram(QByteArray data, QHostAddress receiver, QHostAddress sender, bool alwaysPass)
{
    // Process packet
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
    quint16 reserved = 0;
    quint8 options = 0;
    bool preview = false;
    quint8 *pbuf = (quint8*)data.data();

    switch (ValidateStreamHeader((quint8*)data.data(), data.length(), source_cid, source_name, priority,
            start_code, reserved, sequence, options, universe, slot_count, pdata))
    {
    case e_ValidateStreamHeader::SteamHeader_Invalid:
        // Recieved a packet but not valid. Log and discard
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Invalid Packet";
        return;
    case e_ValidateStreamHeader::SteamHeader_Unknown:
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Unkown Root Vector";
        return;
    case e_ValidateStreamHeader::SteamHeader_Extended:
        qDebug() << "sACNListener" << QThread::currentThreadId() << ": Extended Packet";
        return;
    default:
        break;
    }

    // Unpacks a uint4 from a known big endian buffer
    int root_vect = UpackBUint32((quint8*)pbuf + ROOT_VECTOR_ADDR);

    // Packet for the wrong universe on this socket?
    if(m_universe != universe)
    {
        // Was it unicast? Send to correct listner (if listening)
        if (!alwaysPass && receiver.isMulticast())
        {
            // Log and discard
            qDebug() << "sACNListener" << QThread::currentThreadId() << ": Wrong Universe and is multicast";
            return;
        } else {
            // Unicast, send to releivent listener!
            decltype(sACNManager::getInstance()->getListenerList()) listenerList
                    = sACNManager::getInstance()->getListenerList();
            if (listenerList.contains(universe))
                listenerList[universe].data()->processDatagram(data, receiver, sender);
            return;
        }
    }

    // Listen to preview?
    preview = (PREVIEW_DATA_OPTION == (options & PREVIEW_DATA_OPTION));
    if ((preview) && !Preferences::getInstance()->GetBlindVisualizer())
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

            if(!ps->src_valid)
            {
                // This is a source which is coming back online, so we need to repeat the steps
                // for initial source aquisition
                ps->active.SetInterval(WAIT_OFFLINE + m_ssHLL);
                ps->lastseq = sequence;
                ps->src_cid = source_cid;
                ps->src_valid = true;
                ps->doing_dmx = (start_code == STARTCODE_DMX);
                ps->doing_per_channel = ps->waited_for_dd = false;
                newsourcenotify = false;
                ps->priority_wait.SetInterval(WAIT_PRIORITY);
            }

            if((root_vect == VECTOR_ROOT_E131_DATA) && ((options & STREAM_TERMINATED_OPTION) == STREAM_TERMINATED_OPTION))
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
              (*it)->active.SetInterval(m_ssHLL);  //We factor in the hold last look time here, rather than 0

              if((*it)->doing_per_channel)
                  (*it)->priority_wait.SetInterval(m_ssHLL); //We factor in the hold last look time here, rather than 0

              validpacket = false;
              break;
            }

            //Based on the start code, update the timers
            if(start_code == STARTCODE_DMX)
            {
                //No matter how valid, we got something -- but we'll tweak the interval for any hll change
                (*it)->doing_dmx = true;
                (*it)->active.SetInterval(WAIT_OFFLINE + m_ssHLL);
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
                    (*it)->priority_wait.SetInterval(WAIT_OFFLINE + m_ssHLL);
                    newsourcenotify = true;
                }
                else if((*it)->priority_wait.Expired())
                {
                    (*it)->waited_for_dd = true;
                    (*it)->doing_per_channel = false;
                    (*it)->priority_wait.SetInterval(WAIT_OFFLINE + m_ssHLL);  //In case the source later decides to sent 0xdd packets
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
        ps->active.SetInterval(WAIT_OFFLINE + m_ssHLL);
        ps->lastseq = sequence;
        ps->src_cid = source_cid;
        ps->src_valid = true;
        ps->doing_dmx = (start_code == STARTCODE_DMX);
        //If we are in the sampling period, let all packets through
        if(m_isSampling)
        {
            ps->waited_for_dd = true;
            ps->doing_per_channel = (start_code == STARTCODE_PRIORITY);
            newsourcenotify = true;
            ps->priority_wait.SetInterval(WAIT_OFFLINE + m_ssHLL);
        }
        else
        {
            //If we aren't sampling, we want the earlier logic to set the state
            ps->doing_per_channel = ps->waited_for_dd = false;
            newsourcenotify = false;
            ps->priority_wait.SetInterval(WAIT_PRIORITY);
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
        if(root_vect==VECTOR_ROOT_E131_DATA) protocolVersion = sACNProtocolRelease;
        if(root_vect==VECTOR_ROOT_E131_DATA_DRAFT) protocolVersion = sACNProtocolDraft;
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
            if(ps->fpsTimer.elapsed() >= 1000)
            {
                // Calculate the FPS rate
                ps->fpsTimer.restart();
                ps->fps = ps->fpsCounter;
                ps->fpsCounter = 0;
                ps->source_params_change = true;
            }
            // This is DMX
            // Copy the last array back
            memcpy(ps->last_level_array, ps->level_array, DMX_SLOT_MAX);
            // Fill in the new array
            memset(ps->level_array, 0, DMX_SLOT_MAX);
            memcpy(ps->level_array, pdata, slot_count);
            // Compare the two
            for(int i=0; i<DMX_SLOT_MAX; i++)
            {
                if(ps->level_array[i]!=ps->last_level_array[i])
                {
                    ps->dirty_array[i] |= true;
                    ps->source_levels_change = true;
                }
            }

            // Increment the frame counter - we count only DMX frames
            ps->fpsCounter++;
        }
        else if(start_code == STARTCODE_PRIORITY)
        {
            // Not sending DMX data, so process name and FPS
            if (ps->doing_dmx == false) {

                if(ps->name!=name)
                {
                    ps->name = name;
                    ps->source_params_change = true;
                }

                if(ps->fpsTimer.elapsed() >= 1000)
                {
                    // Calculate the FPS rate
                    ps->fpsTimer.restart();
                    ps->fps = ps->fpsCounter;
                    ps->fpsCounter = 0;
                    ps->source_params_change = true;
                }
            }

            // Copy the last array back
            memcpy(ps->last_priority_array, ps->priority_array, DMX_SLOT_MAX);
            // Fill in the new array
            memset(ps->priority_array, 0, DMX_SLOT_MAX);
            memcpy(ps->priority_array, pdata, slot_count);
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

        // Merge
        performMerge();
    }
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
        for(auto chan: m_monitoredChannels)
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
					&& !(ps->priority_array[address] < priorities[address]) // Not lesser priority
					&& ( (ps->priority_array[address] > 0) || (ps->priority_array[address] == 0 && !ps->doing_per_channel) ) // Priority > 0 if DD
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

            if(ps->src_valid && !ps->active.Expired())
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
        }
        for (auto s : sourceList)
        {
            if(s->level_array[address] > levels[address])
            {
                levels[address] = s->level_array[address];
                m_merged_levels[address].changedSinceLastMerge = (m_merged_levels[address].level != levels[address]);
                m_merged_levels[address].level = levels[address];
                m_merged_levels[address].winningSource = s;
            }
        }
        // Remove the winning source from the list of others
        if(m_merged_levels[address].winningSource)
            m_merged_levels[address].otherSources.remove(m_merged_levels[address].winningSource);
    }


    // Tell people..
    emit levelsChanged();
}

