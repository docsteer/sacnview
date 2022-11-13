#include <cmath>
#include "sacndiscovery.h"
#include "defpack.h"
#include "VHD.h"
#include "preferences.h"
#include "sacnlistener.h"
#include "sacnsender.h"

// Number of universes per discovery page (E1.31:2016 Section 8)
#define UniversesPerPage 512


sACNDiscoveryTX *sACNDiscoveryTX::m_instance = NULL;

void sACNDiscoveryTX::start()
{
    if(!m_instance)
        m_instance = new sACNDiscoveryTX();
}

sACNDiscoveryTX *sACNDiscoveryTX::getInstance()
{
    start();
    return m_instance;
}

sACNDiscoveryTX::sACNDiscoveryTX() : QObject(),
    m_mutex(new QMutex()),
    m_sendTimer(new QTimer(this))
{
    // Socket
    m_sendSock = new sACNTxSocket(Preferences::getInstance()->networkInterface(), this);
    m_sendSock->bind();

    // Setup packet send timer
    m_sendTimer->setObjectName("sACNDiscoveryTX");
    m_sendTimer->setSingleShot(false);
    connect(m_sendTimer, SIGNAL(timeout()), this, SLOT(sendDiscoveryPacket()));
    connect(this, SIGNAL(destroyed()), m_sendTimer, SLOT(deleteLater()));
    m_sendTimer->start(0);
}

sACNDiscoveryTX::~sACNDiscoveryTX()
{
    // Send empty discovery packet
    sendDiscoveryPacket();
}

void sACNDiscoveryTX::sendDiscoveryPacket()
{
    QMutexLocker locker(m_mutex);

    m_sendTimer->setInterval(E131_UNIVERSE_DISCOVERY_INTERVAL);

    // Get list of all senders
    auto senderList = sACNManager::getInstance()->getSenderList();

    // Loop through all CIDs in senders list
    auto cidList = senderList.keys();
    for (auto cid : cidList)
    {
        // Obtain list of universes for CID
        auto universeList = senderList[cid].keys();

        // Remove any that aren't sending!
        QMutableListIterator<quint16> i(universeList);
        while (i.hasNext())
        {
            i.next();
            if (!sACNManager::getInstance()->getSender(i.value(), cid)->isSending())
                i.remove();
        }
        if (universeList.isEmpty())
            continue; // No active universes being sent by this CID

        // Sort list
        std::sort(universeList.begin(), universeList.end());

        // Get sender name associated with CID and first universe in list
        QString senderName;
        if (universeList.count() > 0)
            senderName = sACNManager::getInstance()->getSender(universeList.front(), cid)->name();

        // Get page count
        uint8_t page_count = ceil((double)universeList.count() / UniversesPerPage);

        qDebug() << "DiscoveryTX : CID" << CID::CIDIntoQString(cid) << "universes" << universeList;

        for (uint8_t page = 0; page < page_count; page++)
        {
            // Page Universe List
            auto listStart = page * UniversesPerPage;
            auto listEnd = listStart + UniversesPerPage;
            auto pageUniverseList = universeList.mid(listStart, listEnd);

            // Create buffer
            QByteArray pbuf(
                        E131_UNIVERSE_DISCOVERY_SIZE_MIN + (pageUniverseList.count() * sizeof(pageUniverseList[0]))
                        , 0x00);

            // Root layer
            PackBUint16((quint8*)pbuf.data() + PREAMBLE_SIZE_ADDR, RLP_PREAMBLE_SIZE); // Preamble
            PackBUint16((quint8*)pbuf.data() + POSTAMBLE_SIZE_ADDR, RLP_POSTAMBLE_SIZE); // Post-amble
            memcpy(pbuf.data() + ACN_IDENTIFIER_ADDR, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE); // ACN Ident
            VHD_PackFlags((quint8*)pbuf.data() + ROOT_FLAGS_AND_LENGTH_ADDR, false, false, false); // Flags
            VHD_PackLength((quint8*)pbuf.data() + ROOT_FLAGS_AND_LENGTH_ADDR,
                           pbuf.size() - ROOT_FLAGS_AND_LENGTH_ADDR, false); // Length
            PackBUint32((quint8*)pbuf.data() + ROOT_VECTOR_ADDR, VECTOR_ROOT_E131_EXTENDED); // Vector
            cid.Pack((quint8*)pbuf.data() + CID_ADDR); // CID

            // Framing layer
            VHD_PackFlags((quint8*)pbuf.data() + FRAMING_FLAGS_AND_LENGTH_ADDR, false, false, false); // Flags
            VHD_PackLength((quint8*)pbuf.data() + FRAMING_FLAGS_AND_LENGTH_ADDR,
                           pbuf.size() - FRAMING_FLAGS_AND_LENGTH_ADDR, false); // Length
            PackBUint32((quint8*)pbuf.data() + FRAMING_VECTOR_ADDR, VECTOR_E131_EXTENDED_DISCOVERY); // Vector
            strncpy(pbuf.data() + SOURCE_NAME_ADDR, senderName.toLatin1().constData(), SOURCE_NAME_SIZE - 1); // Source Name

            // Universe discovery layer
            VHD_PackFlags((quint8*)pbuf.data() + DISCO_FLAGS_AND_LENGTH_ADDR, false, false, false); // Flags
            VHD_PackLength((quint8*)pbuf.data() + DISCO_FLAGS_AND_LENGTH_ADDR,
                           pbuf.size() - DISCO_FLAGS_AND_LENGTH_ADDR, false); // Length
            PackBUint32((quint8*)pbuf.data() + DISCO_VECTOR_ADDR, VECTOR_UNIVERSE_DISCOVERY_UNIVERSE_LIST); // Vector
            PackBUint8((quint8*)pbuf.data() + DISCO_PAGE_ADDR, page); // Page
            PackBUint8((quint8*)pbuf.data() + DISCO_LAST_PAGE_ADDR, page_count - 1); // Page Count
            // Universe list
            quint16 idx = 0;

            for (auto universe : pageUniverseList)
            {
               PackBUint16((quint8*)pbuf.data() + DISCO_LIST_UNIVERSE_ADDR + idx, universe);
               idx += sizeof(universe);
            }

            // Send packet
            CIPAddr addr;
            GetUniverseAddress(E131_DISCOVERY_UNIVERSE, addr);
            auto result = m_sendSock->writeDatagram(pbuf, addr.ToQHostAddress(), STREAM_IP_PORT);
            if(result!=pbuf.size())
            {
                qDebug() << "Error sending datagram : " << m_sendSock->errorString();
            }
        }
    }
}



sACNDiscoveryRX *sACNDiscoveryRX::m_instance = NULL;

void sACNDiscoveryRX::start()
{
    if(!m_instance)
        m_instance = new sACNDiscoveryRX();
}

sACNDiscoveryRX *sACNDiscoveryRX::getInstance()
{
    start();
    return m_instance;
}

sACNDiscoveryRX::sACNDiscoveryRX() : QObject(),
    m_mutex(new QMutex()),
    m_listener(new sACNListener(E131_DISCOVERY_UNIVERSE)),
    m_thread(new QThread(this)),
    m_expiredTimer(new QTimer(this))
{
    qDebug() << "DiscoveryRX : Starting listener";
    m_thread->setObjectName("sACNDiscoveryRX");
    m_listener->moveToThread(m_thread);
    connect(m_thread, SIGNAL(started()), m_listener, SLOT(startReception()));
    connect(m_thread, SIGNAL(finished()), m_listener, SLOT(deleteLater()));
    connect(m_thread, SIGNAL(finished()), m_thread, SLOT(deleteLater()));
    m_thread->start();

    m_expiredTimer->setInterval(E131_UNIVERSE_DISCOVERY_INTERVAL + (E131_UNIVERSE_DISCOVERY_INTERVAL / 4)); // Expire after 125% of time
    m_expiredTimer->setObjectName("sACNDiscoveryRX Expired Timer");
    m_expiredTimer->setSingleShot(false);
    connect(m_expiredTimer, SIGNAL(timeout()), this, SLOT(timeoutUniverses()));
    m_expiredTimer->start();
}

sACNDiscoveryRX::~sACNDiscoveryRX()
{
    qDeleteAll(m_discoveryList);
    m_thread->exit();
}

void sACNDiscoveryRX::timeoutUniverses()
{
    QMutexLocker locker(m_mutex);
    QMutableHashIterator<CID, sACNSourceDetail*> discoveryListIterator(m_discoveryList);
    while (discoveryListIterator.hasNext())
    {
        discoveryListIterator.next();
        auto source = discoveryListIterator.value();

        QMutableHashIterator<quint16, ttimer> universeListIterator(source->Universe);
        while (universeListIterator.hasNext())
        {
            universeListIterator.next();
            auto universeTimer = universeListIterator.value();
            if (universeTimer.Expired())
            {
                auto universe = source->Universe.key(universeTimer);
                auto cidString = CID::CIDIntoQString(m_discoveryList.key(source));
                if (source->Universe.remove(universe))
                {
                    qDebug() << "DiscoveryRX : Expired universe - CID" << cidString << ", universe" << universe;
                    emit expiredUniverse(
                                cidString,
                                universe);
                }
            }
        }
        if (!source->Universe.count())
        {
            auto cidString = CID::CIDIntoQString(m_discoveryList.key(source));
            m_discoveryList.remove(
                        m_discoveryList.key(source));
            qDebug() << "DiscoveryRX : Expired Source - CID" << cidString;
            emit expiredSource(cidString);
        }
    }
}

void sACNDiscoveryRX::processPacket(quint8* pbuf, uint buflen)
{
    bool flag1, flag2, flag3;
    quint32 length;
    CID cid;
    quint8 pageCount;
    quint8 pageNum;

    QMutexLocker locker(m_mutex);

    // Check length
    if (!(buflen >= E131_UNIVERSE_DISCOVERY_SIZE_MIN) ||
        !(buflen <= E131_UNIVERSE_DISCOVERY_SIZE_MAX)) return;

    // Root Layer
    if (RLP_PREAMBLE_SIZE != UpackBUint16(pbuf + PREAMBLE_SIZE_ADDR)) return;
    if (RLP_POSTAMBLE_SIZE != UpackBUint16(pbuf + POSTAMBLE_SIZE_ADDR)) return;
    if (0 != memcmp(pbuf + ACN_IDENTIFIER_ADDR, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE)) return;
    VHD_GetFlagLength(pbuf + ROOT_FLAGS_AND_LENGTH_ADDR, flag1, flag2, flag3, length);
    if (flag1 || flag2 || flag3) return;
    if ((buflen - ROOT_FLAGS_AND_LENGTH_ADDR) != length) return;
    if (VECTOR_ROOT_E131_EXTENDED != UpackBUint32(pbuf + ROOT_VECTOR_ADDR)) return;
    cid.Unpack(pbuf + CID_ADDR);

    // Framing layer
    VHD_GetFlagLength(pbuf + FRAMING_FLAGS_AND_LENGTH_ADDR, flag1, flag2, flag3, length);
    if (flag1 || flag2 || flag3) return;
    if ((buflen - FRAMING_FLAGS_AND_LENGTH_ADDR) != length) return;
    if (VECTOR_E131_EXTENDED_DISCOVERY != UpackBUint32(pbuf + FRAMING_VECTOR_ADDR)) return;
    if (!m_discoveryList.contains(cid))
    {
        m_discoveryList[cid] = new sACNSourceDetail;
        qDebug() << "DiscoveryRX : New source - CID" << CID::CIDIntoQString(cid);
        emit newSource(CID::CIDIntoQString(cid));
    }
    m_discoveryList[cid]->Name = QString((char*)pbuf + SOURCE_NAME_ADDR);

    // Universe discovery layer
    VHD_GetFlagLength(pbuf + DISCO_FLAGS_AND_LENGTH_ADDR, flag1, flag2, flag3, length);
    if (flag1 || flag2 || flag3) return;
    if ((buflen - DISCO_FLAGS_AND_LENGTH_ADDR) != length) return;
    if (VECTOR_UNIVERSE_DISCOVERY_UNIVERSE_LIST != UpackBUint32(pbuf + DISCO_VECTOR_ADDR)) return;
    pageNum = UpackBUint8(pbuf + DISCO_PAGE_ADDR);
    pageCount = UpackBUint8(pbuf + DISCO_LAST_PAGE_ADDR);
    Q_UNUSED(pageNum);
    Q_UNUSED(pageCount);
    if(buflen != E131_UNIVERSE_DISCOVERY_SIZE_MIN)
    {
        for (quint16 n = 0; n < buflen - DISCO_LIST_UNIVERSE_ADDR; n += sizeof(quint16))
        {
            quint16 universe = UpackBUint16(pbuf + DISCO_LIST_UNIVERSE_ADDR + n);
            if (!m_discoveryList[cid]->Universe.contains(universe))
            {
                qDebug() << "DiscoveryRX : New universe - CID" << CID::CIDIntoQString(cid) << ", universe" << universe;
                emit newUniverse(CID::CIDIntoQString(cid), universe);
            }
            m_discoveryList[cid]->Universe[universe].SetInterval(std::chrono::milliseconds(E131_UNIVERSE_DISCOVERY_INTERVAL));
        }
    }
    else
    {
        qDebug() << "DiscoveryRX : Discovery packet received with empty universe list - CID" << CID::CIDIntoQString(cid);
    }
}
