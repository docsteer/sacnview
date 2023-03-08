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

#include "sacnsender.h"
#include <vector>
#include <set>
#include <algorithm>
#include <memory>

#include "CID.h"
#include "ipaddr.h"
#include "tock.h"
#include "streamcommon.h"
#include "securesacn.h"

#include <QUuid>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QCoreApplication>

#include "preferences.h"

using namespace std::chrono_literals;
#if defined(Q_OS_UNIX)
constexpr auto tickLoopInterval = 1ms;
#elif defined(Q_OS_WIN)
constexpr auto tickLoopInterval = 0ms; // Don't use on Windows
#endif
const auto tickLoopPriority = QThread::LowestPriority;

sACNSentUniverse::sACNSentUniverse(int universe) :
    m_isSending(false),
    m_handle(0),
    m_slotCount(DMX_SLOT_MAX),
    m_priority(100),
    m_name("New Source"),
    m_universe(universe),
    m_priorityMode(pmPER_SOURCE_PRIORITY),
    m_version(sACNProtocolRelease),
    m_checkTimeoutTimer(Q_NULLPTR),
    m_synchronization(NOT_SYNCHRONIZED_VALUE),
    m_minSendFreq(E131_DATA_KEEP_ALIVE_FREQUENCY),
    m_maxSendFreq(E1_11::MAX_REFRESH_RATE_HZ)
{}

sACNSentUniverse::~sACNSentUniverse()
{
    stopSending();

    if(m_checkTimeoutTimer)
    {
        m_checkTimeoutTimer->stop();
        delete m_checkTimeoutTimer;
    }
}

void sACNSentUniverse::startSending(bool preview)
{
    Q_ASSERT(isSending() == false);

    quint8 options = NO_OPTIONS_VALUE;

    CStreamServer *streamServer = CStreamServer::getInstance();
    if(m_cid.isNull())
        m_cid = CID::CreateCid();

    if (preview)
        options += PREVIEW_DATA_OPTION;

    m_maxSendFreq = std::clamp(
        m_maxSendFreq,
        m_minSendFreq,
        Preferences::getInstance()->GetTXRateOverride() ?
                    std::numeric_limits<decltype(m_maxSendFreq)>::max() : E1_11::MAX_REFRESH_RATE_HZ);

    CIPAddr unicastAddress;
    if(!m_unicastAddress.isNull())
        unicastAddress = CIPAddr(m_unicastAddress);

    // DMX Data (0x00) server
    streamServer->CreateUniverse(
                m_cid, qPrintable(m_name), m_priority, m_synchronization, options, STARTCODE_DMX,
                m_universe, m_slotCount, m_slotData, m_handle, m_minSendFreq, m_maxSendFreq,
                unicastAddress, m_version);
    streamServer->setSecurePassword(m_handle, m_password);
    streamServer->SetUniverseDirty(m_handle);

    // Per-Address (0xdd) server
    if(m_priorityMode == pmPER_ADDRESS_PRIORITY)
    {
        quint8 *pslots;
        streamServer->CreateUniverse(
                    m_cid, qPrintable(m_name), m_priority, NOT_SYNCHRONIZED_VALUE, options, STARTCODE_PRIORITY,
                    m_universe, m_slotCount, pslots, m_priorityHandle, E1_11::MIN_REFRESH_RATE_HZ, m_maxSendFreq,
                    unicastAddress, m_version);
        memcpy(pslots,
               m_perChannelPriorities,
               std::min(static_cast<size_t>(m_slotCount), sizeof(m_perChannelPriorities)));
        streamServer->SetUniverseDirty(m_priorityHandle);
    }

    m_isSending = true;

    int seconds = Preferences::getInstance()->GetNumSecondsOfSacn();
    if(seconds>0)
    {
        m_checkTimeoutTimer = new QTimer(this);
        connect(m_checkTimeoutTimer, SIGNAL(timeout()), this, SLOT(doTimeout()));
        m_checkTimeoutTimer->setSingleShot(true);
        m_checkTimeoutTimer->setInterval(1000 * seconds);
        m_checkTimeoutTimer->start();
    }
}


void sACNSentUniverse::stopSending()
{
    CStreamServer::getInstance()->DestroyUniverse(m_handle);
    m_handle = 0;
    m_isSending = false;

    if(m_priorityMode == pmPER_ADDRESS_PRIORITY)
    {
        CStreamServer::getInstance()->DestroyUniverse(m_priorityHandle);
        m_priorityHandle = 0;
    }
}

void sACNSentUniverse::setLevel(quint16 address, quint8 value)
{
    Q_ASSERT(address<DMX_SLOT_MAX);
    if (address>=m_slotCount)
        return;
    if(isSending())
    {
        if (m_slotData[address] != value)
        {
            m_slotData[address] =  value;
            CStreamServer::getInstance()->SetUniverseDirty(m_handle);
        }
    }
}

void sACNSentUniverse::setLevelRange(quint16 start, quint16 end, quint8 value)
{
    Q_ASSERT(start<DMX_SLOT_MAX);
    Q_ASSERT(end<DMX_SLOT_MAX);

    if (start > end)
        std::swap(start, end);
    if (start>=m_slotCount || end>=m_slotCount)
        return;
    if(isSending())
    {
        size_t len = end-start+1;
        std::unique_ptr<quint8[]> pTemp(new quint8[DMX_SLOT_MAX]);
        memset(pTemp.get() + start, value, len);
        if (memcmp(pTemp.get() + start, m_slotData + start, len) != 0)
        {
            memset(m_slotData + start, value, end-start+1);
            CStreamServer::getInstance()->SetUniverseDirty(m_handle);
        }
    }
}

void sACNSentUniverse::setLevel(const quint8 *data, int len, int start)
{
    Q_ASSERT((start + len)<=DMX_SLOT_MAX);
    len = qMin(len, static_cast<decltype(len)>(m_slotCount));
    if(isSending())
    {
        if (memcmp(data, m_slotData + start, static_cast<size_t>(len)) != 0)
        {
            memcpy(m_slotData + start, data, static_cast<size_t>(len));
            CStreamServer::getInstance()->SetUniverseDirty(m_handle);
        }
    }
}

void sACNSentUniverse::setVerticalBar(quint16 index, quint8 level)
{
    Q_ASSERT(index<32);

    if(isSending())
    {
        memset(m_slotData, 0, m_slotCount);
        for(int i=0; i<16; i++)
        {
            m_slotData[(i*32)+index] = level;
        }

        CStreamServer::getInstance()->SetUniverseDirty(m_handle);
    }
}


void sACNSentUniverse::setHorizontalBar(quint16 index, quint8 level)
{
    Q_ASSERT(index<16);

    if(isSending())
    {
        memset(m_slotData, 0, m_slotCount);
        memset(m_slotData + 32*index, level, 32);

        CStreamServer::getInstance()->SetUniverseDirty(m_handle);
    }
}

void sACNSentUniverse::setName(const QString &name)
{
    auto tmpStr = name.trimmed();
    tmpStr.truncate(MAX_SOURCE_NAME_LEN);
    m_name = tmpStr;
    if(isSending())
    {
        QByteArray arr = name.toUtf8();
        CStreamServer::getInstance()->setUniverseName(m_handle ,arr.constData());
    }
}

void sACNSentUniverse::setPriorityMode(PriorityMode mode)
{
    m_priorityMode = mode;
}

void sACNSentUniverse::setPerChannelPriorities(quint8 *priorities)
{
   memcpy(m_perChannelPriorities, priorities, sizeof(m_perChannelPriorities));
}

void sACNSentUniverse::setPerSourcePriority(quint8 priority)
{
    m_priority = priority;
    if(m_isSending)
    {
         CStreamServer::getInstance()->setUniversePriority(m_handle, priority);
    }
}

void sACNSentUniverse::setProtocolVersion(StreamingACNProtocolVersion version)
{
    m_version = version;
}

void sACNSentUniverse::copyLevels(quint8 *dest)
{
    memcpy(dest, m_slotData, MAX_DMX_ADDRESS);
}

void sACNSentUniverse::setUniverse(int universe)
{
    if(m_universe==universe) return;
    m_universe = universe;
    if(m_isSending)
    {
        stopSending();
        startSending();
    }

    emit universeChange();
}

void sACNSentUniverse::setSynchronization(quint16 address)
{
    m_synchronization = address;
    if(m_isSending)
    {
         CStreamServer::getInstance()->setSynchronizationAddress(m_handle, m_synchronization);
    }

    emit synchronizationChange();
}

void sACNSentUniverse::setCID(CID cid)
{
    if(m_cid==cid) return;
    m_cid = cid;
    if(m_isSending)
    {
        stopSending();
        startSending();
    }

    emit cidChange();
}

void sACNSentUniverse::setSlotCount(quint16 slotCount)
{
    if(m_slotCount==slotCount) return;
    m_slotCount = slotCount;
    if(m_isSending)
    {
        stopSending();
        startSending();
    }

    emit slotCountChange();
}

void sACNSentUniverse::setSecurePassword(const QString &password)
{
    if(m_password==password) return;
    m_password = password;
    if(m_isSending)
    {
        stopSending();
        startSending();
    }

    emit passwordChange();
}

void sACNSentUniverse::setSendFrequency(float minimum, float maximum)
{
    if(minimum==m_minSendFreq && maximum==m_maxSendFreq) return;
    m_minSendFreq = minimum;
    m_maxSendFreq = maximum;
    if(isSending())
    {
        CStreamServer::getInstance()->setSendFrequency(m_handle, m_minSendFreq, m_maxSendFreq);
    }

    emit sendFrequencyChange();
}

void sACNSentUniverse::doTimeout()
{
    delete m_checkTimeoutTimer;
    m_checkTimeoutTimer = Q_NULLPTR;
    stopSending();
    emit sendingTimeout();
    qDebug() << "Source " << this->name() << " timeout";
}

CStreamServer *CStreamServer::m_instance = Q_NULLPTR;

CStreamServer *CStreamServer::getInstance()
{
    if(!m_instance)
        m_instance = new CStreamServer();
    return m_instance;
}

void CStreamServer::shutdown()
{
    if(m_instance)
        m_instance->deleteLater();
    m_instance = Q_NULLPTR;
}

CStreamServer::CStreamServer() :
    m_thread_stop(false)
{
    m_sendsock = new sACNTxSocket(Preferences::getInstance()->networkInterface());

    m_sendsock->bind();

    m_thread = new QThread();
    connect(m_thread, SIGNAL(started()), this, SLOT(TickLoop()));
    connect(m_thread, &QThread::finished, this, &QThread::deleteLater);
    this->moveToThread(m_thread);
    m_thread->setObjectName(QString("Interface %1 TX").arg(m_sendsock->multicastInterface().name()));
    m_thread->start();
    m_thread->setPriority(tickLoopPriority);
}

CStreamServer::~CStreamServer()
{
    //Clean up the sequence numbers
    for(seqiter it3 = m_seqmap.begin(); it3 != m_seqmap.end(); ++it3)
    {
        if(it3->second.second)
        {
            delete it3->second.second;
            it3->second.second = Q_NULLPTR;
        }
    }

    m_thread_stop = true;
    m_thread->quit();
    m_thread->wait();
}


//Returns a pointer to the storage location for the universe, adding if
//need be.
//The newly-added location contains sequence number 0.
quint8* CStreamServer::GetPSeq(const CID &cid, quint16 universe)
{
    cidanduniverse identifier(cid, universe);

    seqiter it = m_seqmap.find(identifier);
    if(it != m_seqmap.end())
    {
        ++it->second.first;
        return it->second.second;
    }
    quint8 * p = new quint8;
    if(!p)
        return Q_NULLPTR;
    *p = 0;
    m_seqmap.insert(std::pair<cidanduniverse, seqref>(identifier, seqref(1, p)));
    return p;
}

//Removes a reference to the storage location for the universe, removing
//completely if need be.
void CStreamServer::RemovePSeq(const CID &cid, quint16 universe)
{
    cidanduniverse identifier(cid, universe);

    seqiter it = m_seqmap.find(identifier);

    if(it != m_seqmap.end())
    {
        --it->second.first;
        if(it->second.first <= 0)
        {
            delete it->second.second;
            m_seqmap.erase(it);
        }
    }
}


void CStreamServer::TickLoop()
{
    qDebug() << "sACNSender" << QThread::currentThreadId()
             << ": Starting with priority"
             << this->thread()->priority()
             << " and tickLoopInterval"
             << std::chrono::duration_cast<std::chrono::milliseconds>(tickLoopInterval).count() << "ms";

    Q_ASSERT(this->thread()->priority() == tickLoopPriority);

    while (!m_thread_stop) {
        if (tickLoopInterval.count())
            QThread::usleep(std::chrono::duration_cast<std::chrono::microseconds>(tickLoopInterval).count());

        QMutexLocker locker(&m_writeMutex);

        for(auto it = m_multiverse.begin(); it != m_multiverse.end(); ++it)
        {
            //Too soon to send?
            //E1.31:2016 6.6.1
            if (!it->min_interval.Expired())
                continue;

            //If this has been send 3 times (or more?) with a termination flag
            //then it's time to kill it
            if(it->num_terminates >= 3)
            {
                DoDestruction(it->handle);
            }

            //If valid, either a dirty, still about to supress rate, or send_interval will cause a send
            if(it->psend && (it->isdirty || it->num_suppress_remaining || it->send_interval.Expired()))
            {
                // E1.31:2016 6.6.2 (clause 1) logic
                if(it->isdirty && it->start_code == STARTCODE_DMX)
                    it->num_suppress_remaining = 3;
                else if(it->num_suppress_remaining)
                    --it->num_suppress_remaining;

                //Add the sequence number and send
                quint8 *pseq = GetPSeq(it->cid, it->number);
                SetStreamHeaderSequence(it->psend, *pseq, it->version == sACNProtocolDraft);
                (*pseq)++;

                // Pathway Connectivity Secure DMX Protocol
                if (it->version == sACNProtocolPathwaySecure)
                    PathwaySecure::ApplyStreamSecurity(it->psend, it->sendsize, it->cid, it->password);

                // Write to the wire
                quint64 result = m_sendsock->writeDatagram( (char*)it->psend, it->sendsize, it->sendaddr, STREAM_IP_PORT);

                if(result!=it->sendsize)
                {
                    qDebug() << "Error sending datagram : " << m_sendsock->errorString();
                }

                if(GetStreamTerminated(it->psend))
                {
                    it->num_terminates++;
                }

                //Finally, set the timing/dirtiness for the next interval
                it->isdirty = false;
                it->send_interval.Reset();
                it->min_interval.Reset();
            }
        }
    } // Loop

    qDebug() << "sACNSender" << QThread::currentThreadId() << ": Stopping";
}


//Use this to create a universe for a source cid, startcode, etc.
//If it returns true, two parameters are filled in: The data buffer for the values that can
//  be manipulated directly, and the handle to use when calling the rest of these functions.
//Note that a universe number of 0 is invalid.
//Set reserved to be 0 and options to either be 0 or PREVIEW_DATA_OPTION (can be changed later
//  by OptionsPreviewData).
//if ignore_inactivity_logic is set to false (the default for DMX), Tick will handle sending the
//  3 identical packets at the lower frequency that sACN requires.  It sends these packets at
//  send_intervalms intervals (again defaulted for DMX).  Note that even if you are not using the
//  inactivity logic, send_intervalms expiry will trigger a resend of the current universe packet.
//Data on this universe will not be initially sent until marked dirty.
bool CStreamServer::CreateUniverse(
        const CID& source_cid, const char* source_name, quint8 priority,
        quint16 synchronization, quint8 options, quint8 start_code,
        quint16 universe, quint16 slot_count, quint8*& pslots, uint& handle,
        float minimum_send_rate, float maximum_send_rate ,
        CIPAddr unicastAddress,
        StreamingACNProtocolVersion version)
{
    QMutexLocker locker(&m_writeMutex);
    if(universe == 0)
        return false;

   //Before we attempt to create the universe, make sure we can create the buffer.
    uint sendsize = slot_count;
    switch (version)
    {
        default:
        case sACNProtocolUnknown:
            qFatal("Attempting to set unknown protocol version (0x%x)", version);
            return false;

        case sACNProtocolDraft:
            sendsize += DRAFT_STREAM_HEADER_SIZE;
            break;

        case sACNProtocolRelease:
            sendsize += STREAM_HEADER_SIZE;
            break;

        case sACNProtocolPathwaySecure:
            sendsize += STREAM_HEADER_SIZE + PathwaySecure::RootLayer::POSTAMBLE_SIZE;
            break;
    }

    quint8* pbuf = new quint8 [sendsize];
    if(!pbuf)
        return false;
    memset(pbuf, 0, sendsize);

    // Find a slot if we can
    bool foundSpace = false;
    for(unsigned int i=0; i<m_multiverse.size(); i++)
    {
        if(m_multiverse[i].psend == NULL)
        {
            handle = i;
            foundSpace = true;
            break;
        }
    }

    if(!foundSpace)
    {
        // Allocate a new universe
        struct universe tmp;
        handle = static_cast<uint>(m_multiverse.size());
        m_multiverse.push_back(tmp);
    }

    //Init the state
    m_multiverse[handle].number = universe;
    m_multiverse[handle].handle = handle;
    m_multiverse[handle].start_code = start_code;
    m_multiverse[handle].isdirty = false;
    m_multiverse[handle].num_terminates=0;
    setSendFrequency(handle, minimum_send_rate, maximum_send_rate);
    m_multiverse[handle].version = version;
    m_multiverse[handle].cid = source_cid;

    CIPAddr addr;
    GetUniverseAddress(universe, addr);

    if(unicastAddress != CIPAddr())
    {
        addr = unicastAddress;
    }

    m_multiverse[handle].sendaddr = addr.ToQHostAddress();

    switch (version) {
    case sACNProtocolUnknown:
        return false;

    case sACNProtocolDraft:
        InitStreamHeaderForDraft(pbuf, source_cid, source_name, priority, synchronization, options, start_code, universe, slot_count);
        break;

    case sACNProtocolRelease:
        InitStreamHeader(pbuf, source_cid, source_name, priority, synchronization, options, start_code, universe, slot_count);
        break;

    case sACNProtocolPathwaySecure:
        PathwaySecure::InitStreamHeader(pbuf, source_cid, source_name, priority, synchronization, options, start_code, universe, slot_count);
        break;
    }

    m_multiverse[handle].psend = pbuf;
    m_multiverse[handle].sendsize = sendsize;
    if(version == sACNProtocolDraft)
        pslots = pbuf + DRAFT_STREAM_HEADER_SIZE;
    else
        pslots = pbuf + STREAM_HEADER_SIZE;
    return true;
}

//After you add data to the data buffer, call this to trigger the data send on
//the next Tick boundary.
//Otherwise, the data won't be sent until the inactivity or send_interval time.
void CStreamServer::SetUniverseDirty(uint handle)
{
    QMutexLocker locker(&m_writeMutex);
    m_multiverse[handle].isdirty = true;
}

//In the event that you want to send out a message for a particular
//universe (and start code) in between ticks, call this function.
//This does not affect the dirty bit for the universe, inactivity count,
//etc, and the tick will still operate normally when called.
//This is not thread safe with Tick -- Don't call when Tick is called
void CStreamServer::SendUniverseNow(uint handle)
{
    //Basically, a copy of the sending part of Tick

    universe* puni = &m_multiverse[handle];
    quint8 *pseq = GetPSeq(puni->cid, puni->number);
    SetStreamHeaderSequence(puni->psend, *pseq, puni->version == sACNProtocolDraft);
    (*pseq)++;

    m_sendsock->writeDatagram((char*) puni->psend, puni->sendsize, puni->sendaddr, STREAM_IP_PORT);
}

//Use this to destroy a priority universe.
void CStreamServer::DEBUG_DESTROY_PRIORITY_UNIVERSE(uint handle)
{
  DoDestruction(handle);
}

//Use this to destroy a universe.
//this does invalidate the pslots array that CreateUniverse returned, so do not access
//that memory after or during this call.
//Not Thread Safe -- Don't call when Tick is called
void CStreamServer::DestroyUniverse(uint handle)
{
    if(handle < m_multiverse.size())
    {
        QMutexLocker locker(&m_writeMutex);
        if (m_multiverse[handle].version == sACNProtocolDraft)
        {
            DoDestruction(handle);
        } else {
            SetStreamTerminated(m_multiverse[handle].psend, true);
        }
    }
}

//Perform the logical destruction and cleanup of a universe and its related
//objects.
void CStreamServer::DoDestruction(uint handle)
{
    if((handle < m_multiverse.size()) && m_multiverse[handle].psend)
    {
        m_multiverse[handle].num_terminates = 0;
        delete [] m_multiverse[handle].psend;
        m_multiverse[handle].psend = NULL;
    }
}


//sets the preview_data bit of the options field
void CStreamServer::OptionsPreviewData(uint handle, bool preview)
{
  SetPreviewData(m_multiverse[handle].psend, preview);
}

//sets the stream_terminated bit of the options field
void CStreamServer::OptionsStreamTerminated(uint handle, bool terminated)
{
  SetStreamTerminated(m_multiverse[handle].psend, terminated);
}

void CStreamServer::setUniverseName(uint handle, const char *name)
{
    if(m_multiverse[handle].psend)
    {
        strncpy((char *)m_multiverse[handle].psend + SOURCE_NAME_ADDR,
                name,
                m_multiverse[handle].version == sACNProtocolDraft ? DRAFT_SOURCE_NAME_SIZE : SOURCE_NAME_SIZE);
    }
}


void CStreamServer::setUniversePriority(uint handle, quint8 priority)
{
    if(m_multiverse[handle].psend && m_multiverse[handle].version == sACNProtocolDraft)
    {
        m_multiverse[handle].psend[DRAFT_PRIORITY_ADDR] = priority;
    }
    else if(m_multiverse[handle].psend)
    {
        m_multiverse[handle].psend[PRIORITY_ADDR] = priority;
    }
}

void CStreamServer::setSynchronizationAddress(uint handle, quint16 address)
{
    if(m_multiverse[handle].psend && !(m_multiverse[handle].version == sACNProtocolDraft))
    {
        m_multiverse[handle].psend[SYNC_ADDR] = address;
    }
}

void CStreamServer::setSecurePassword(uint handle, const QString &password)
{
   m_multiverse[handle].password = password;
}

void CStreamServer::setSendFrequency(uint handle, float minimum, float maximum)
{
    if(handle < m_multiverse.size()) {
        if (maximum <= 0)
            maximum = E1_11::MAX_REFRESH_RATE_HZ;

        if (minimum <= 0 || minimum > maximum)
            minimum = E1_11::MIN_REFRESH_RATE_HZ;

        typedef std::chrono::microseconds period_t;
        m_multiverse[handle].send_interval.SetInterval(
                    period_t(static_cast<period_t::rep>(period_t::period().den / minimum)));
        m_multiverse[handle].min_interval.SetInterval(
                    period_t(static_cast<period_t::rep>(period_t::period().den / maximum)));

        m_multiverse[handle].send_interval.Reset();
        m_multiverse[handle].min_interval.Reset();
    }
}

