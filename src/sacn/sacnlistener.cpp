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
#include "deftypes.h"
#include "defpack.h"
#include <QDebug>
#include <QThread>
#include <QPoint>
#include <QSharedPointer>
#include <QWeakPointer>


//The amount of ms to wait before a source is considered offline or
//has stopped sending per-channel-priority packets
#define WAIT_OFFLINE 2500

//The amount of ms to wait before determining that a newly discovered source is not doing per-channel-priority
#define WAIT_PRIORITY 1500

//The time during which to sample
#define SAMPLE_TIME 1500

sACNListener::sACNListener(QObject *parent) : QThread(parent)
{
    // PUT EVERYTHING IN RUN!
}


sACNListener::~sACNListener()
{
}

void sACNListener::run()
{
    qDebug() << "sACNListener started in thread: " << QThread::currentThreadId();
    m_mergeTimer = Q_NULLPTR;
    m_initalSampleTimer = Q_NULLPTR;
    m_ssHLL = 1000;
    m_merged_levels.reserve(512);
    for(int i=0; i<512; i++)
        m_merged_levels << sACNMergedAddress();

    m_elapsedTime.start();
    m_mergesPerSecond = 0;
    m_mergesPerSecondTimer.start();

    exec();
}

void sACNListener::startReception(int universe)
{
    m_universe = universe;
    m_isSampling = true;
    // Clear the levels array
    memset(&m_last_levels, -1, 512);

    // Listen multicast
    m_sockets.push_back(new sACNRxSocket(this));
    if (m_sockets.back()->bindMulticast(universe)) {
        connect(m_sockets.back(), SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    } else {
       // Failed to bind,
       m_sockets.pop_back();
    }

    // Listen unicast
    m_sockets.push_back(new sACNRxSocket(this));
    if (m_sockets.back()->bindUnicast()) {
        connect(m_sockets.back(), SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    } else {
       // Failed to bind, mostly expected for unicast.
       m_sockets.pop_back();
    }

    m_initalSampleTimer = new QTimer(this);
    m_initalSampleTimer->setInterval(100);
    connect(m_initalSampleTimer, SIGNAL(timeout()), this, SLOT(checkSampleExpiration()));
    m_initalSampleTimer->start();

    // Merge is performed whenever the thread has time
    m_mergeTimer = new QTimer(this);
    m_mergeTimer->setInterval(1);
    connect(m_mergeTimer, SIGNAL(timeout()), this, SLOT(performMerge()));
    connect(m_mergeTimer, SIGNAL(timeout()), this, SLOT(checkSourceExpiration()));
    m_mergeTimer->start();
}


void sACNListener::checkSampleExpiration()
{
    if(this->m_isSampling && m_sampleTimer.Expired())
    {
        m_isSampling = false;
        qDebug() << "Sampling has ended";
        m_initalSampleTimer->stop();
        m_initalSampleTimer->deleteLater();
        m_initalSampleTimer = Q_NULLPTR;
    }
}

void sACNListener::checkSourceExpiration()
{
    char cidstr [CID::CIDSTRINGBYTES];
    for(std::vector<sACNSource *>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
    {
        if((*it)->src_valid)
        {
            if((*it)->active.Expired())
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
                qDebug() << "Source stopped sending per-channel priority" << cidstr;
            }
        }
    }
}

void sACNListener::readPendingDatagrams()
{
    // Check all sockets
    foreach (sACNRxSocket* m_socket, m_sockets)
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

void sACNListener::processDatagram(QByteArray data, QHostAddress receiver, QHostAddress sender)
{
    // Process packet
    CID source_cid;
    uint1 start_code;
    uint1 sequence;
    uint2 universe;
    uint2 slot_count;
    uint1* pdata;
    char source_name [SOURCE_NAME_SIZE];
    uint1 priority;
    /*
     * These only apply to the ratified version of the spec, so we will hardwire
     * them to be 0 just in case they never get set.
    */
    uint2 reserved = 0;
    uint1 options = 0;
    bool preview = false;
    uint1 *pbuf = (uint1*)data.data();

    if(!ValidateStreamHeader((uint1*)data.data(), data.length(), source_cid, source_name, priority,
            start_code, reserved, sequence, options, universe, slot_count, pdata))
    {
        // Recieved a packet but not valid. Log and discard
        qDebug() << "Invalid Packet";
        return;
    }

    // Unpacks a uint4 from a known big endian buffer
    int root_vect = UpackB4((uint1*)pbuf + ROOT_VECTOR_ADDR);

    // Packet for the wrong universe on this socket?
    if(m_universe != universe)
    {
        // Was it unicast? Send to correct listner (if listening)
        if (receiver.isMulticast())
        {
            // Log and discard
            qDebug() << "Wrong Universe and is multicast";
            return;
        } else {
            // Unicast, send to releivent listener!
            const QHash<int, QWeakPointer<sACNListener> > listenerList = sACNManager::getInstance()->getListenerList();
            if (listenerList.contains(universe))
                listenerList[universe].data()->processDatagram(data, receiver, sender);
            return;
        }
    }

    // Listen to preview?
    preview = (PREVIEW_DATA_OPTION == (options & PREVIEW_DATA_OPTION));
    if ((preview) && !Preferences::getInstance()->GetBlindVisualizer())
    {
        qDebug() << "Ignore preview";
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

            if((root_vect == ROOT_VECTOR) && ((options & 0x40) == 0x40))
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
            int1 result = ((int1)sequence) - ((int1)((*it)->lastseq));
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
                validpacket = false;
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
        qDebug() << "Source coming up, not processing packet";
        return;
    }

    if(!foundsource)  //Add a new source to the list
    {
        ps = new sACNSource();

        m_sources.push_back(ps);

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
        qDebug() << "Found new source name " << source_name;
        m_mergeAll = true;
        emit sourceFound(ps);
    }

    if (newsourcenotify)
    {
        // This is a source that came back online
        qDebug() << "Source came back name " << source_name;
        m_mergeAll = true;
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
        if(root_vect==ROOT_VECTOR) protocolVersion = sACNProtocolRelease;
        if(root_vect==DRAFT_ROOT_VECTOR) protocolVersion = sACNProtocolDraft;
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
            memcpy(ps->last_level_array, ps->level_array, 512);
            // Fill in the new array
            memset(ps->level_array, 0, 512);
            memcpy(ps->level_array, pdata, slot_count);
            // Compare the two
            for(int i=0; i<512; i++)
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
            // Copy the last array back
            memcpy(ps->last_priority_array, ps->priority_array, 512);
            // Fill in the new array
            memset(ps->priority_array, 0, 512);
            memcpy(ps->priority_array, pdata, slot_count);
            // Compare the two
            for(int i=0; i<512; i++)
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
    }
}

void sACNListener::performMerge()
{
    //array of addresses to merge. to prevent duplicates and because you can have
    //an odd collection of addresses, addresses[n] would be 'n' for the value in question
    // and -1 if not required
    int addresses_to_merge[512];
    int number_of_addresses_to_merge = 0;

    memset(addresses_to_merge, -1, sizeof(int) * 512);

    foreach(int chan, m_monitoredChannels)
    {
        QPointF data;
        data.setX(m_elapsedTime.nsecsElapsed()/1000000.0);
        data.setY(mergedLevels().at(chan).level);
        emit dataReady(chan, data);
    }

    if(m_mergesPerSecondTimer.hasExpired(1000))
    {
        m_mergesPerSecond = m_mergeCounter;
        m_mergeCounter = 0;
        m_mergesPerSecondTimer.start();
    }

    m_mergeCounter++;



    // Step one : find any addresses which have changed
    if(m_mergeAll) // Act like all addresses changed
    {
        number_of_addresses_to_merge = 512;
        for(int i=0; i<512; i++)
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
            for(int i=0; i<512; i++)
            {
                if(ps->dirty_array[i])
                {
                    addresses_to_merge[i] = i;
                    number_of_addresses_to_merge++;
                }
            }
            // Clear the flags
            memset(ps->dirty_array, 0 , 512);
            ps->source_levels_change = false;
        }
    }

    if(number_of_addresses_to_merge == 0) return; // Nothing to do

    // Clear out the sources list for all the affected channels, we'll be refreshing it

    int skipCounter = 0;
    for(int i=0; i < 512 && i<(number_of_addresses_to_merge + skipCounter); i++)
    {
        m_merged_levels[i].changedSinceLastMerge = false;
        if(addresses_to_merge[i] == -1) {
            ++skipCounter;
            continue;
        }
        sACNMergedAddress *pAddr = &m_merged_levels[addresses_to_merge[i]];
        pAddr->otherSources.clear();
    }

    // Find the highest priority source for each address we need to work on

    int levels[512];
    memset(&levels, -1, sizeof(levels));

    int priorities[512];
    memset(&priorities, -1, sizeof(priorities));

    QMultiMap<int, sACNSource*> addressToSourceMap;


    for(std::vector<sACNSource *>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
    {
        sACNSource *ps = *it;

        // Find the highest priority for the address, ignoring priorities of zero
        if(ps->src_valid && !ps->active.Expired() && !ps->doing_per_channel)
        {
            // Set the priority array for sources which are not doing per-channel
            memset(ps->priority_array, ps->priority, sizeof(ps->priority_array));
        }

        skipCounter = 0;
        for(int i=0; i < 512 && i<(number_of_addresses_to_merge + skipCounter); i++)
        {
            if(addresses_to_merge[i] == -1) {
               ++skipCounter;
                continue;
            }
            sACNMergedAddress *pAddr = &m_merged_levels[addresses_to_merge[i]];
            int address = addresses_to_merge[i];

            if (ps->src_valid  && !ps->active.Expired() && ps->priority_array[address] > priorities[address] && ps->priority_array[address]>0)
            {
                // Sources of higher priority
                priorities[address] = ps->priority_array[address];
                addressToSourceMap.remove(address);
                addressToSourceMap.insert(address, ps);
            }
            if (ps->src_valid  && !ps->active.Expired() && ps->priority_array[address] == priorities[address] && ps->priority_array[address]>0)
            {
                addressToSourceMap.insert(address, ps);
            }
            if(ps->src_valid && !ps->active.Expired())
                pAddr->otherSources << ps;
        }
    }



    // Next, find highest level for the highest prioritized sources
    skipCounter = 0;
    for(int i=0; i < 512 && i<(number_of_addresses_to_merge + skipCounter); i++)
    {
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
        foreach(sACNSource *s, sourceList)
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

