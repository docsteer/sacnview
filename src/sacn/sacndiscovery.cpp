#include <cmath>
#include "sacndiscovery.h"
#include "defpack.h"
#include "VHD.h"
#include "preferences.h"
#include "streamingacn.h"
#include "sacnsender.h"

// Number of universes per discovery page (E1.31:2016 Section 8)
#define UniversesPerPage 512

sacndiscoveryTX::sacndiscoveryTX(QObject *parent) : QObject(parent),
    m_mutex(new QMutex()),
    m_sendTimer(new QTimer(this))
{
    // Socket
    m_sendSock = new sACNTxSocket(Preferences::getInstance()->networkInterface(), this);
    m_sendSock->bindMulticast();

    // Setup packet send timer
    m_sendTimer->setObjectName("E131_UNIVERSE_DISCOVERY_INTERVAL");
    m_sendTimer->setInterval(E131_UNIVERSE_DISCOVERY_INTERVAL);
    m_sendTimer->setSingleShot(false);
    connect(m_sendTimer, SIGNAL(timeout()), this, SLOT(sendDiscoveryPacket()));
    connect(this, SIGNAL(destroyed()), m_sendTimer, SLOT(deleteLater()));
    m_sendTimer->start();
}

sacndiscoveryTX::~sacndiscoveryTX()
{
    // Send empty discovery packet
    sendDiscoveryPacket();
}

void sacndiscoveryTX::sendDiscoveryPacket()
{
    QMutexLocker locker(m_mutex);

    // Get list of all senders
    auto senderList = sACNManager::getInstance()->getSenderList();

    // Loop through all CIDs in senders list
    auto cidList = senderList.keys();
    for (auto cid : cidList)
    {
        // Obtain list of universes for CID
        auto universeList = senderList[cid].keys();

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
                           (pbuf.size() - 1) - (ROOT_FLAGS_AND_LENGTH_ADDR + 1), false); // Length
            PackBUint32((quint8*)pbuf.data() + ROOT_VECTOR_ADDR, VECTOR_ROOT_E131_EXTENDED); // Vector
            cid.Pack((quint8*)pbuf.data() + CID_ADDR); // CID

            // Framing layer
            VHD_PackFlags((quint8*)pbuf.data() + FRAMING_FLAGS_AND_LENGTH_ADDR, false, false, false); // Flags
            VHD_PackLength((quint8*)pbuf.data() + FRAMING_FLAGS_AND_LENGTH_ADDR,
                           (pbuf.size() - 1) - (FRAMING_FLAGS_AND_LENGTH_ADDR + 1), false); // Length
            PackBUint32((quint8*)pbuf.data() + FRAMING_VECTOR_ADDR, VECTOR_E131_EXTENDED_DISCOVERY); // Vector
            strncpy(pbuf.data() + SOURCE_NAME_ADDR, senderName.toLatin1().constData(), SOURCE_NAME_SIZE - 1); // Source Name

            // Universe discovery layer
            VHD_PackFlags((quint8*)pbuf.data() + DISCO_FLAGS_AND_LENGTH_ADDR, false, false, false); // Flags
            VHD_PackLength((quint8*)pbuf.data() + DISCO_FLAGS_AND_LENGTH_ADDR,
                           (pbuf.size() - 1) - (DISCO_FLAGS_AND_LENGTH_ADDR + 1), false); // Length
            PackBUint32((quint8*)pbuf.data() + DISCO_VECTOR_ADDR, VECTOR_UNIVERSE_DISCOVERY_UNIVERSE_LIST); // Vector
            PackBUint8((quint8*)pbuf.data() + DISCO_PAGE_ADDR, page); // Page
            PackBUint8((quint8*)pbuf.data() + DISCO_PAGE_ADDR, page_count - 1); // Page Count
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
