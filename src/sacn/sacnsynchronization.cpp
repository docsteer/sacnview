#include "sacnsynchronization.h"
#include "defpack.h"
#include "VHD.h"
#include "sacnlistener.h"
#include "sacnsender.h"
#include "preferences.h"

sACNSynchronizationTX::sACNSynchronizationTX(CID cid, tsyncAddress syncAddress) : QObject(),
    m_cid(cid),
    m_syncAddress(syncAddress)
{
    // Create base packet
    createSynchronizationPacket();

    // Socket
    m_sendSock = new sACNTxSocket(Preferences::Instance().networkInterface(), this);
    m_sendSock->bind();
}

sACNSynchronizationTX::~sACNSynchronizationTX() {}

void sACNSynchronizationTX::createSynchronizationPacket() {
    m_pbuf.resize(E131_UNIVERSE_SYNCHRONIZATION_SIZE_MAX);
    m_pbuf.fill(0x00, E131_UNIVERSE_SYNCHRONIZATION_SIZE_MAX);

    // Root layer
    PackBUint16((quint8*)m_pbuf.data() + PREAMBLE_SIZE_ADDR, RLP_PREAMBLE_SIZE); // Preamble
    PackBUint16((quint8*)m_pbuf.data() + POSTAMBLE_SIZE_ADDR, RLP_POSTAMBLE_SIZE); // Post-amble
    memcpy(m_pbuf.data() + ACN_IDENTIFIER_ADDR, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE); // ACN Ident
    VHD_PackFlags((quint8*)m_pbuf.data() + ROOT_FLAGS_AND_LENGTH_ADDR, false, false, false); // Flags
    VHD_PackLength((quint8*)m_pbuf.data() + ROOT_FLAGS_AND_LENGTH_ADDR,
                   m_pbuf.size() - ROOT_FLAGS_AND_LENGTH_ADDR, false); // Length
    PackBUint32((quint8*)m_pbuf.data() + ROOT_VECTOR_ADDR, VECTOR_ROOT_E131_EXTENDED); // Vector
    m_cid.Pack((quint8*)m_pbuf.data() + CID_ADDR); // CID

    // Framing layer
    VHD_PackFlags((quint8*)m_pbuf.data() + SYNC_FLAGS_AND_LENGTH_ADDR, false, false, false); // Flags
    VHD_PackLength((quint8*)m_pbuf.data() + SYNC_FLAGS_AND_LENGTH_ADDR,
                   m_pbuf.size() - SYNC_FLAGS_AND_LENGTH_ADDR, false); // Length
    PackBUint32((quint8*)m_pbuf.data() + SYNC_VECTOR_ADDR, VECTOR_E131_EXTENDED_SYNCHRONIZATION); // Vector
    PackBUint8((quint8*)m_pbuf.data() + SYNC_SEQ_NUM_ADDR, m_sequence); // Sequence
    PackBUint16((quint8*)m_pbuf.data() + SYNC_SYNCHRONIZATION_ADDRESS, m_syncAddress); // Synchronization Address
    PackBUint16((quint8*)m_pbuf.data() + SYNC_RESERVED, 0x00); // Reserved
}


void sACNSynchronizationTX::sendSynchronizationPacket()
{
    QMutexLocker locker(&m_mutex);

    // Sequence
    PackBUint8((quint8*)m_pbuf.data() + SYNC_SEQ_NUM_ADDR, ++m_sequence);

    // Send packet
    CIPAddr addr;
    GetUniverseAddress(m_syncAddress, addr);
    auto result = m_sendSock->writeDatagram(m_pbuf, addr.ToQHostAddress(), STREAM_IP_PORT);
    if(result!=m_pbuf.size())
    {
        qDebug() << "Error sending datagram : " << m_sendSock->errorString();
    }
}


sACNSynchronizationRX *sACNSynchronizationRX::m_instance = NULL;

void sACNSynchronizationRX::start() {
    if(!m_instance)
        m_instance = new sACNSynchronizationRX();
}

sACNSynchronizationRX *sACNSynchronizationRX::getInstance() {
    start();
    return m_instance;
}

sACNSynchronizationRX::sACNSynchronizationRX() : QObject(),
    m_thread(new QThread(this)),
    m_expiredTimer(new QTimer(this))
{
    qDebug() << "SynchronizationRX : Starting RX";
    m_thread->setObjectName("sACNSynchronizationRX");
    connect(m_thread, SIGNAL(finished()), m_thread, SLOT(deleteLater()));
    m_thread->start();

    m_expiredTimer->setInterval(E131_NETWORK_DATA_LOSS_TIMEOUT + (E131_NETWORK_DATA_LOSS_TIMEOUT / 4)); // Expire after 125% of time
    m_expiredTimer->setObjectName("sACNSynchronizationRX Expired Timer");
    m_expiredTimer->setSingleShot(false);
    connect(m_expiredTimer, SIGNAL(timeout()), this, SLOT(timeoutSyncAddresses()));
    m_expiredTimer->start();
}

sACNSynchronizationRX::~sACNSynchronizationRX() {
    m_thread->exit();
}

void sACNSynchronizationRX::timeoutSyncAddresses() {
    QMutexLocker locker(&m_mutex);

    for (auto syncAddr : getSynchronizationAddresses()) {
        for (auto cid : getSynchronizationSources(syncAddr).keys()) {
            const auto &source = getSynchronizationSources(syncAddr).value(cid);
            if (source.dataLoss.Expired()) {
                removeSource(syncAddr, cid);
            }
        }
    }
}

void sACNSynchronizationRX::processPacket(quint8* pbuf, uint buflen, QHostAddress destination, QHostAddress sender)
{
    bool flag1, flag2, flag3;
    quint32 length;
    CID cid;

    QMutexLocker locker(&m_mutex);

    // Check length
    if (buflen < E131_UNIVERSE_SYNCHRONIZATION_SIZE_MIN) return;

    // Root Layer
    if (RLP_PREAMBLE_SIZE != UpackBUint16(pbuf + PREAMBLE_SIZE_ADDR)) return;
    if (RLP_POSTAMBLE_SIZE != UpackBUint16(pbuf + POSTAMBLE_SIZE_ADDR)) return;
    if (0 != memcmp(pbuf + ACN_IDENTIFIER_ADDR, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE)) return;
    VHD_GetFlagLength(pbuf + ROOT_FLAGS_AND_LENGTH_ADDR, flag1, flag2, flag3, length);
    if (flag1 || flag2 || flag3) return;
    if ((buflen - ROOT_FLAGS_AND_LENGTH_ADDR) != length) return;
    if (VECTOR_ROOT_E131_EXTENDED != UpackBUint32(pbuf + ROOT_VECTOR_ADDR)) return;
    cid.Unpack(pbuf + CID_ADDR);
    if (cid.isNull()) return;

    // Synchronization Packet Framing Layer
    VHD_GetFlagLength(pbuf + SYNC_FLAGS_AND_LENGTH_ADDR, flag1, flag2, flag3, length);
    if (flag1 || flag2 || flag3) return;
    if ((buflen - SYNC_FLAGS_AND_LENGTH_ADDR) != length) return;
    if (VECTOR_E131_EXTENDED_SYNCHRONIZATION != UpackBUint32(pbuf + SYNC_VECTOR_ADDR)) return;

    quint8 sequence = UpackBUint8(pbuf + SYNC_SEQ_NUM_ADDR);

    quint16 reserved = UpackBUint16(pbuf + SYNC_RESERVED);
    Q_UNUSED(reserved);

    tsyncAddress syncAddress = UpackBUint16(pbuf + SYNC_SYNCHRONIZATION_ADDRESS);

    if (syncAddress == 0) return;
    if (destination.isMulticast()) {
        CIPAddr addr;
        GetUniverseAddress(syncAddress, addr);
        if (addr.ToQHostAddress() != destination) return;
    }

    if (!m_synchronizationSources.contains(syncAddress) || !m_synchronizationSources.value(syncAddress).contains(cid)) {
        addSource(syncAddress, cid, sender, sequence);
    } else {
        if (!m_synchronizationSources[syncAddress][cid].sequence.checkSeq(sequence)) return;
    }

    m_synchronizationSources[syncAddress][cid].sender = sender;
    m_synchronizationSources[syncAddress][cid].dataLoss.SetInterval(E131_NETWORK_DATA_LOSS_TIMEOUT);
    m_synchronizationSources[syncAddress][cid].fps->newFrame();

    emit synchronize(syncAddress);
}

void sACNSynchronizationRX::addSource(tsyncAddress syncAddress, CID cid, QHostAddress sender, quint8 sequence) {
    if (!m_synchronizationSources.contains(syncAddress)) {
        m_synchronizationSources[syncAddress].insert(cid, sACNSourceDetail(sender, sequence));
        qDebug() << "SynchronizationRX : New Sync Address" << syncAddress;
        emit newSyncAddress(syncAddress);
        qDebug() << "SynchronizationRX : Sync Address" << syncAddress << "New source - CID" << cid;
        emit newSource(syncAddress, cid);
    }

    if (!m_synchronizationSources.value(syncAddress).contains(cid)) {
        m_synchronizationSources[syncAddress].insert(cid, sACNSourceDetail(sender, sequence));
        qDebug() << "SynchronizationRX : Sync Address" << syncAddress << "New source - CID" << cid;
        emit newSource(syncAddress, cid);
    }
}

void sACNSynchronizationRX::removeSource(tsyncAddress syncAddress, CID cid) {
    if (m_synchronizationSources.contains(syncAddress)) {
        m_synchronizationSources[syncAddress].remove(cid);
        qDebug() << "SynchronizationRX : Expired source" << cid;
        emit expiredSource(syncAddress, cid);

        if (!m_synchronizationSources[syncAddress].count()) {
            m_synchronizationSources.remove(syncAddress);
            qDebug() << "SynchronizationRX : Expired sync address" << syncAddress;
            emit expiredSyncAddress(syncAddress);
        }
    }
}
