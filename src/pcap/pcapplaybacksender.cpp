#include "pcapplaybacksender.h"
#ifdef Q_OS_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <pcap.h>
#include <QDebug>
#include <QtEndian>
#include "preferences.h"
#include "sacn/streamingacn.h"
#include "sacn/sacnlistener.h"
#include "ethernetstrut.h"

pcapplaybacksender::pcapplaybacksender(QString FileName) :
    m_filename(FileName),
    m_pcap_in(Q_NULLPTR),
    m_pcap_out(Q_NULLPTR),
    m_running(false),
    m_shutdown(false)
{
    if (!openFile()) {
        closeFile();
    }
}

pcapplaybacksender::~pcapplaybacksender()
{
    m_running = false;

    closeFile();

    QMutexLocker mutexOut(&m_pcap_out_Mutex);
    if (m_pcap_out) {
        pcap_close(m_pcap_out);
        m_pcap_out = Q_NULLPTR;
    }
}

void pcapplaybacksender::run()
{
    /* Open output adapter */
    {
        QMutexLocker mutexOut(&m_pcap_out_Mutex);

        char errbuf[PCAP_ERRBUF_SIZE];

        // Get required interface
        QNetworkInterface iface = Preferences::Instance().networkInterface();

        // Find all usable interfaces
        pcap_if_t *alldevs;
        if (pcap_findalldevs(&alldevs, errbuf) == -1)
        {
            emit error(tr("%1").arg(errbuf));
            return;
        }

        // See which avaliable interface matches required interface
        for(pcap_if_t *dev = alldevs; dev != NULL; dev = dev->next)
        {
            // Already have an interface
            if (m_pcap_out) { break; }

            for (pcap_addr *addr = dev->addresses; addr != NULL; addr = addr->next)
            {
                // Already have an interface
                if (m_pcap_out) { break; }

                if (addr->addr->sa_family == AF_INET) {
                    // IPv4
                    struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr->addr;

                    // Is this address assigned to the interface required?
                    foreach (QNetworkAddressEntry ifaceAddr, iface.addressEntries())
                    {
                        // Already have an interface
                        if (m_pcap_out) { break; }

                        if (ifaceAddr.ip().protocol() == QAbstractSocket::IPv4Protocol)
                        {
                            // Selected interface contains the IP of the required interface                  
                            #ifdef Q_OS_UNIX
                                QHostAddress requiredIP(qFromBigEndian<quint32>(ipv4->sin_addr.s_addr));
                            #else
                                QHostAddress requiredIP(qFromBigEndian<quint32>(ipv4->sin_addr.S_un.S_addr));
                            #endif
                            if (requiredIP == ifaceAddr.ip())
                            {
                               qDebug() << "PCap Playback: Output" << dev->name << requiredIP.toString();

                               // Yes this interface!
                               m_pcap_out = pcap_open_live(dev->name, 0, 1, 0, errbuf);
                               if (!m_pcap_out)
                               {
                                   emit error(tr("%1\n%2").arg(errbuf, iface.humanReadableName()));
                                   return;
                               }
                            }
                        }
                    }
                }
            }
        }

        // Clean up
        pcap_freealldevs(alldevs);

        // Did we get an interface?
        if (!m_pcap_out) {
            emit error(tr("Unable to open required interface\n%1").arg(iface.humanReadableName()));
            return;
        }
    }

    while (!m_shutdown)
    {
        if (m_running && m_pcap_out && m_pcap_in)
        {
            QMutexLocker mutexIn(&m_pcap_in_Mutex);
            QMutexLocker mutexOut(&m_pcap_out_Mutex);

            /* Get next packet and send at correct interval */
            struct pcap_pkthdr *pkt_header;
            const u_char *pkt_data;
            if (pcap_next_ex(m_pcap_in, &pkt_header, &pkt_data) == 1)
            {
                // Create QT datagram to send to self
                QNetworkDatagram pkt_datagram = createDatagram(m_pcap_in, pkt_header, pkt_data);

                // Wait until time
                if (!m_pktLastTime.isNull())
                {
                    QTime pkt_time(0,0,0);
                    pkt_time = pkt_time.addSecs(static_cast<int>(pkt_header->ts.tv_sec));
                    pkt_time = pkt_time.addMSecs(static_cast<int>(pkt_header->ts.tv_usec / 1000));
                    QThread::msleep(m_pktLastTime.msecsTo(pkt_time));
                }

                if (!m_running) { continue; }
                /*
                 * Send to wire
                 *
                 * We use libpcap to retain sender IP and MAC when sending on the wire
                 * MULITICAST_LOOP is not enabled, so this is not recived by me.
                 * we need a seperate action to display this locally
                */
                if (pcap_sendpacket(m_pcap_out, pkt_data, pkt_header->caplen) == 0) {
                    emit packetSent();
                } else {
                    emit error(QString("%1").arg(pcap_geterr(m_pcap_out)));
                    m_running = false;
                    emit sendingFinished();
                    return;
                }

                /* Send to me
                 *
                 * Find listener for this universe and pass the packet on
                 *
                 */
                {

                    CIPAddr addr;
                    for (auto &weakListener : sACNManager::Instance().getListenerList()) {
                        sACNManager::tListener listener(weakListener);
                        if (listener) {
                            GetUniverseAddress(listener->universe(), addr);
                            if (QHostAddress(addr.GetV4Address()) == pkt_datagram.destinationAddress()) {
                                listener->processDatagram(
                                            pkt_datagram.data(),
                                            pkt_datagram.destinationAddress(),
                                            pkt_datagram.senderAddress());
                                break;
                            }
                        }
                    }
                }

                // Save time
                m_pktLastTime = QTime(0,0,0);
                m_pktLastTime = m_pktLastTime.addSecs(static_cast<int>(pkt_header->ts.tv_sec));
                m_pktLastTime = m_pktLastTime.addMSecs((pkt_header->ts.tv_usec / 1000));
            } else
            {
                // Done!
                m_running = false;
                emit sendingFinished();
            }
        } else {
            // Noop
            QThread::msleep(100);
        }
    }

    exec();
}

void pcapplaybacksender::quit()
{
    m_running = false;
    m_shutdown = true;
}

bool pcapplaybacksender::openFile()
{
    // Ensure already closed
    closeFile();

    /* Open the capture file */
    QMutexLocker mutexIn(&m_pcap_in_Mutex);
    char errbuf[PCAP_ERRBUF_SIZE];
    m_pcap_in = pcap_open_offline(m_filename.toUtf8(), errbuf);
    if (!m_pcap_in) {
        emit error(tr("Error opening %1\n%2").arg(QDir::toNativeSeparators(m_filename), errbuf));
        return false;
    }

    qDebug() << "PCap Playback: Input" << m_filename;

    /* Filter to only include sACN packets */
    struct bpf_program fcode;
    const char packet_filter[] = sacn_packet_filter;
    if (pcap_compile(m_pcap_in, &fcode, packet_filter, 1, 0xffffff) != 0)
    {
        closeFile();
        emit error(tr("Error opening %1\npcap_compile failed").arg(QDir::toNativeSeparators(m_filename)));
        return false;
    }
    if (pcap_setfilter(m_pcap_in, &fcode)<0)
    {
        closeFile();
        emit error(tr("Error opening %1\npcap_setfilter failed").arg(QDir::toNativeSeparators(m_filename)));
        return false;
    }

    return true;
}

void pcapplaybacksender::closeFile()
{
    m_running = false;
    QMutexLocker mutexIn(&m_pcap_in_Mutex);
    m_pktLastTime = QTime();
    if (m_pcap_in) {
        pcap_close(m_pcap_in);
        m_pcap_in = Q_NULLPTR;
    }

    emit sendingClosed();
}

bool pcapplaybacksender::isRunning()
{
    return m_running;
}

void pcapplaybacksender::play()
{
    m_running = true;
}

void pcapplaybacksender::pause()
{
    m_running = false;
}

void pcapplaybacksender::reset()
{
    closeFile();
    openFile();
}

QNetworkDatagram pcapplaybacksender::createDatagram(pcap_t *hpcap, pcap_pkthdr_t *pkt_header, const unsigned char *pkt_data)
{
    /*
     * Disect packet and create QNetworkDatagram
     * Hardware addresses are lost
    */

    int linkType = pcap_datalink(hpcap);
    switch (linkType)
    {
        case DLT_EN10MB: // Ethernet
        {
            // Layer 2
            const struct sniff_ethernet *ethernet;
            if (pkt_header->caplen < sizeof(ethernet))
            {
                qDebug() << "PCapPlayback: Missing complete layer 2";
                return QNetworkDatagram();;
            }
            ethernet = (struct sniff_ethernet*)(pkt_data);
            if (qFromBigEndian(ethernet->ether_type) != 0x0800)
            {
                qDebug() << "PCapPlayback: Layer 2 ethertype not IPv4";
                return QNetworkDatagram();;
            }; // IPv4, no vlan

            // Layer 3
            const struct sniff_ip *ip;
            if (pkt_header->caplen < (sizeof(ethernet) + sizeof(ip)))
            {
                qDebug() << "PCapPlayback: Missing complete layer 3";
                return QNetworkDatagram();;
            }
            ip = (struct sniff_ip*)(ethernet + 1);

            // Layer 4
            const struct sniff_udp *udp;
            if (pkt_header->caplen < (sizeof(ethernet) + sizeof(ip) + sizeof(udp)))
            {
                qDebug() << "PCapPlayback: Missing layer 4";
                return QNetworkDatagram();;
            }
            udp = (struct sniff_udp*)(ip + 1);

            // Payload
            const char *payload;
            unsigned int payload_len = 0;
            payload = (const char *)(udp + 1);
            payload_len = qFromBigEndian(udp->uh_ulen) - sizeof(udp);
            if (pkt_header->caplen < (sizeof(ethernet) + sizeof(ip) + sizeof(udp) + payload_len))
            {
                qDebug() << "PCapPlayback: Missing Payload";
                return QNetworkDatagram();;
            }

            // Create Datagram
            /*
             * Remember: network byte order = big endian
            */
            QNetworkDatagram ret;
            #ifdef Q_OS_UNIX
                const QHostAddress senderHost = QHostAddress(qFromBigEndian<quint32>(ip->ip_src.s_addr));
            #else
                const QHostAddress senderHost = QHostAddress(qFromBigEndian<quint32>(ip->ip_src.S_un.S_addr));
            #endif
            quint16 senderPort = qFromBigEndian<quint16>(udp->uh_sport);
            ret.setSender(senderHost, senderPort);

            #ifdef Q_OS_UNIX
                const QHostAddress destHost = QHostAddress(qFromBigEndian<quint32>(ip->ip_dst.s_addr));
            #else
                const QHostAddress destHost = QHostAddress(qFromBigEndian<quint32>(ip->ip_dst.S_un.S_addr));
            #endif

            quint16 destPort = qFromBigEndian<quint16>(udp->uh_dport);
            ret.setDestination(destHost, destPort);

            ret.setData(QByteArray(payload, payload_len));

            return ret;
        }

        default:
            // Other, can not handle
            return QNetworkDatagram();
    }
}
