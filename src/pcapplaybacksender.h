#ifndef PCAPPLAYBACKSENDER_H
#define PCAPPLAYBACKSENDER_H

#include <QObject>
#include <QThread>
#include <QDir>
#include <QTime>
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
    void run();

signals:
    void error(QString errorMessage);
    void packetSent();
    void sendingFinished();

public slots:
    void play();
    void pause();
    void reset();

private:
    QThread *m_thread;
    QString m_filename;

    typedef struct pcap pcap_t;
    pcap_t *m_pcap_in;
    pcap_t *m_pcap_out;
    bool m_running;
    QTime m_pktLastTime;
};

#endif // PCAPPLAYBACKSENDER_H
