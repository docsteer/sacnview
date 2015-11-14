// Copyright (c) 2015 Electronic Theatre Controls, http://www.etcconnect.com
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

#include "sacnlistener.h"
#include "streamcommon.h"
#include "preferences.h"
#include "deftypes.h"
#include "defpack.h"
#include <QDebug>


//The amount of ms to wait before a source is considered offline or
//has stopped sending per-channel-priority packets
#define WAIT_OFFLINE 2500

//The amount of ms to wait before determining that a newly discovered source is not doing per-channel-priority
#define WAIT_PRIORITY 1500

//The time during which to sample
#define SAMPLE_TIME 1500

sACNListener::sACNListener(QObject *parent) : QObject(parent)
{
    m_socket = new QUdpSocket;
    m_versionSpec = sACNProtocolDraft | sACNProtocolRelease;
    m_ssHLL = 1000;
    m_merged_levels.reserve(512);
    for(int i=0; i<512; i++)
        m_merged_levels << sACNMergedAddress();
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
}

sACNListener::~sACNListener()
{
    delete m_socket;
}

void sACNListener::startReception(int universe)
{
    m_universe = universe;
    m_isSampling = true;
    // Clear the levels array
    memset(&m_last_levels, -1, 512);
    CIPAddr addr;
    GetUniverseAddress(universe, addr);

    quint16 port = addr.GetIPPort();
    m_socket->bind(QHostAddress(QHostAddress::AnyIPv4),
                   port,
                   QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);

    QNetworkInterface iface = Preferences::getInstance()->networkInterface();

    m_socket->joinMulticastGroup(QHostAddress(addr.GetV4Address()), iface);

    m_initalSampleTimer = new QTimer(this);
    m_initalSampleTimer->setInterval(100);
    connect(m_initalSampleTimer, SIGNAL(timeout()), this, SLOT(checkSampleExpiration()));
    m_initalSampleTimer->start();

    m_mergeTimer = new QTimer(this);
    m_mergeTimer->setInterval(100);
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
                qDebug() << "Lost source " << cidstr;
            }
            else if ((*it)->doing_per_channel && (*it)->priority_wait.Expired())
            {
                CID::CIDIntoString((*it)->src_cid, cidstr);
                (*it)->doing_per_channel = false;
                emit sourceChanged(*it);
                qDebug() << "Source stopped sending per-channel priority" << cidstr;
            }
        }
    }
}

void sACNListener::readPendingDatagrams()
{
    while(m_socket->hasPendingDatagrams())
    {
    QByteArray data;
    data.resize(m_socket->pendingDatagramSize());
    QHostAddress sender;
    quint16 senderPort;

    m_socket->readDatagram(data.data(), data.size(),
                            &sender, &senderPort);

    //Process packet
    CID source_cid;
    uint1 start_code;
    uint1 sequence;
    uint2 universe;
    uint2 slot_count;
    uint1* pdata;
    char source_name [SOURCE_NAME_SIZE];
    uint1 priority;
    //These only apply to the ratified version of the spec, so we will hardwire
    //them to be 0 just in case they never get set.
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

    //Unpacks a uint4 from a known big endian buffer
    int root_vect = UpackB4((uint1*)pbuf + ROOT_VECTOR_ADDR);

    //Determine which version the packet is, and if we want it.
    if(root_vect == ROOT_VECTOR && !(m_versionSpec & sACNProtocolRelease))
        return;

    if(root_vect == DRAFT_ROOT_VECTOR && !(m_versionSpec & sACNProtocolDraft))
       return;

    if(m_universe != universe)
    {
        // Packet for the wrong universe on this address
        // Log and discard
        qDebug() << "Wrong Universe";
        return;
    }

    preview = (0x80 == (options & 0x80));

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
        qDebug() << "Invalid Packet";
        return;
    }

    if(!foundsource)  //Add a new source to the list
    {
        ps = new sACNSource();

        m_sources.push_back(ps);

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
        emit sourceFound(ps);
    }

    if (newsourcenotify)
    {
        // This is a source that came back online
        qDebug() << "Source came back name " << source_name;
        emit sourceChanged(ps);
    }


    //Finally, Process the buffer
    if(validpacket)
    {
        bool changed = false;
        QString name = QString::fromUtf8(source_name);

        if(ps->ip != sender)
        {
            ps->ip = sender;
            changed = true;
        }

        StreamingACNProtocolVersion protocolVersion = sACNProtocolUnknown;
        if(root_vect==ROOT_VECTOR) protocolVersion = sACNProtocolRelease;
        if(root_vect==DRAFT_ROOT_VECTOR) protocolVersion = sACNProtocolDraft;
        if(ps->protocol_version!=protocolVersion)
        {
            ps->protocol_version = protocolVersion;
            changed = true;
        }

        if(start_code == STARTCODE_DMX)
        {
            if(ps->name!=name)
            {
                ps->name = name;
                changed = true;
            }
            if(ps->isPreview != preview)
            {
                ps->isPreview = preview;
                changed = true;
            }
            if(ps->priority != priority)
            {
                ps->priority = priority;
                changed = true;
            }
            if(ps->fpsTimer.elapsed() >= 1000)
            {
                // Calculate the FPS rate
                ps->fpsTimer.restart();
                ps->fps = ps->fpsCounter;
                ps->fpsCounter = 0;
                changed = true;
            }
            //use DMX
            memcpy(ps->level_array, pdata, slot_count);

            // Increment the frame counter - we count only DMX frames
            ps->fpsCounter++;
        }
        else if(start_code == STARTCODE_PRIORITY)
        {
            memcpy(ps->priority_array, pdata, slot_count);
        }

        if(changed)
        {
            qDebug() << "Source Parameters Changed";
            emit sourceChanged(ps);
        }
    }
    }
}

void sACNListener::performMerge()
{
    int levels[512];
    memset(&levels, -1, sizeof(levels));

    int priorities[512];
    memset(&priorities, -1, sizeof(priorities));

    QMultiMap<int, sACNSource*> addressToSourceMap;


    // First step : find the highest priority for every address
    for(std::vector<sACNSource *>::iterator it = m_sources.begin(); it != m_sources.end(); ++it)
    {
        sACNSource *ps = *it;
        for(int i=0; i<512; i++)
        {
            sACNMergedAddress *pAddr = &m_merged_levels[i];
            pAddr->otherSources.clear();


            // Find the highest priority for the address, ignoring priorities of zero
            if(ps->src_valid && !ps->active.Expired() && !ps->doing_per_channel)
            {
                // Set the priority array for sources which are not doing per-channel
                memset(ps->priority_array, ps->priority, sizeof(ps->priority_array));
            }
            if (ps->src_valid  && !ps->active.Expired() && ps->priority_array[i] > priorities[i] && ps->priority_array[i]>0)
            {
                // Sources of higher priority
                priorities[i] = ps->priority_array[i];
                addressToSourceMap.remove(i);
                addressToSourceMap.insert(i, ps);
                pAddr->otherSources.append(ps);
            }
            if (ps->src_valid  && !ps->active.Expired() && ps->priority_array[i] == priorities[i])
            {
                addressToSourceMap.insert(i, ps);
                pAddr->otherSources.append(ps);
            }
        }
    }

    // Next, find highest level for the highest prioritized sources

    for(int address=0; address<512; address++)
    {
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
                m_merged_levels[address].level = levels[address];
                m_merged_levels[address].winningSource = s;
            }
        }
    }


    // Finally, see if anything changed
    int result = memcmp(levels, m_last_levels, sizeof(levels));
    if(result!=0)
    {
        memcpy(m_last_levels, levels, sizeof(levels));
        emit levelsChanged();
    }
}

