#ifndef PCAPPLAYBACKSENDER_H
#define PCAPPLAYBACKSENDER_H

#include <QObject>
#include <QThread>
#include <QDir>
#include <QTime>
#include <QMutexLocker>
#include <QNetworkDatagram>
#include "sacn/sacnsocket.h"

#define sacn_packet_filter "ip and udp and dst port 5568 and dst net 239.255.0.0 mask 255.255.6.0"

class pcapplaybacksender : public QThread
{
    Q_OBJECT

public:
    explicit pcapplaybacksender(QString FileName);
    ~pcapplaybacksender();

    bool isRunning();

private:
    bool openFile();
    void closeFile();
    void run();

signals:
    void error(QString errorMessage);
    void packetSent();
    void sendingFinished();
    void sendingClosed();

public slots:
    void play();
    void pause();
    void reset();
    void quit();

private:
    QThread *m_thread;
    QString m_filename;

    typedef struct pcap pcap_t;
    pcap_t *m_pcap_in;
    QMutex m_pcap_in_Mutex;
    pcap_t *m_pcap_out;
    QMutex m_pcap_out_Mutex;
    bool m_running;
    bool m_shutdown;
    QTime m_pktLastTime;

    typedef struct pcap_pkthdr pcap_pkthdr_t;
    QNetworkDatagram createDatagram(pcap_t *hpcap, pcap_pkthdr_t *pkt_header, const unsigned char *pkt_data);
};

#endif // PCAPPLAYBACKSENDER_H
