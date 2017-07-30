#include "pcapplaybacksender.h"
#include <pcap.h>
#include <QDebug>
#include <QtEndian>
#include "preferences.h"

#include "ethernetstrut.h"

pcapplaybacksender::pcapplaybacksender(QString FileName) :
    m_running(false),
    m_shutdown(false),
    m_filename(FileName),
    m_pcap_in(Q_NULLPTR),
    m_pcap_out(Q_NULLPTR)
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
        QNetworkInterface iface = Preferences::getInstance()->networkInterface();

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
                            if (QHostAddress(qFromBigEndian<quint32>(ipv4->sin_addr.S_un.S_addr)) == ifaceAddr.ip())
                            {
                               qDebug() << "PCap Playback: Output" << dev->name << QHostAddress(qFromBigEndian<quint32>(ipv4->sin_addr.S_un.S_addr));

                               // Yes this interface!
                               m_pcap_out = pcap_open_live(dev->name, 0, 1, 0, errbuf);
                               if (!m_pcap_out)
                               {
                                   emit error(tr("%1\n%2").arg(errbuf).arg(iface.humanReadableName()));
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
            error(tr("Unable to open required interface\n%1").arg(iface.humanReadableName()));
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
//                /* Disect returned packet */
//                // Headers
//                const struct sniff_ethernet *ethernet;
//                const struct sniff_ip *ip;
//                const struct sniff_udp *udp;
//                const char *payload;
//                unsigned int payload_len = 0;

//                int linkType = pcap_datalink(m_pcap_in);
//                switch (linkType)
//                {
//                    case DLT_EN10MB:
//                    {
//                        // Ethernet

//                        // Layer 2
//                        if (pkt_header->caplen < sizeof(ethernet))
//                        {
//                            qDebug() << "PCapPlayback: Missing complete layer 2";
//                            continue;
//                        }
//                        ethernet = (struct sniff_ethernet*)(pkt_data);
//                        if (qFromBigEndian(ethernet->ether_type) != 0x0800)
//                        {
//                            qDebug() << "PCapPlayback: Layer 2 ethertype not IPv4";
//                            continue;
//                        }; // IPv4 Only

//                        // Layer 3
//                        if (pkt_header->caplen < (sizeof(ethernet) + sizeof(ip)))
//                        {
//                            qDebug() << "PCapPlayback: Missing complete layer 3";
//                            continue;
//                        }
//                        ip = (struct sniff_ip*)(ethernet + 1);

//                        // Layer 4
//                        if (pkt_header->caplen < (sizeof(ethernet) + sizeof(ip) + sizeof(udp)))
//                        {
//                            qDebug() << "PCapPlayback: Missing layer 4";
//                            continue;
//                        }
//                        udp = (struct sniff_udp*)(ip + 1);

//                        // Payload
//                        payload = (const char *)(udp + 1);
//                        payload_len = qFromBigEndian(udp->uh_ulen) - sizeof(udp);
//                        if (pkt_header->caplen < (sizeof(ethernet) + sizeof(ip) + sizeof(udp) + payload_len))
//                        {
//                            qDebug() << "PCapPlayback: Missing Payload";
//                            continue;
//                        }
//                        break;
//                    }

//                    default:
//                        // Other, can not handle
//                        continue;
//                }

                // Wait until time
                if (!m_pktLastTime.isNull())
                {
                    QTime pkt_time(0,0,0);
                    pkt_time = pkt_time.addSecs(pkt_header->ts.tv_sec);
                    pkt_time = pkt_time.addMSecs((pkt_header->ts.tv_usec / 1000));
                    QThread::msleep(m_pktLastTime.msecsTo(pkt_time));
                }

                // Send
                if (!m_running) { continue; }
                if (pcap_sendpacket(m_pcap_out, pkt_data, pkt_header->caplen) == 0) {
                    emit packetSent();
                } else {
                    error(QString("%1").arg(pcap_geterr(m_pcap_out)));
                    m_running = false;
                    emit sendingFinished();
                    return;
                }

                // Save time
                m_pktLastTime = QTime(0,0,0);
                m_pktLastTime = m_pktLastTime.addSecs(pkt_header->ts.tv_sec);
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
        emit error(tr("Error opening %1\n%2").arg(QDir::toNativeSeparators(m_filename)).arg(errbuf));
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
