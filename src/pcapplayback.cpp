#include "pcapplayback.h"
#include "ui_pcapplayback.h"
#include "pcapplaybacksender.h"
#include "pcap.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QThread>
#include <QDebug>

PcapPlayback::PcapPlayback(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PcapPlayback),
    sender(NULL)
{
    ui->setupUi(this);
}

PcapPlayback::~PcapPlayback()
{
    delete ui;
}

void PcapPlayback::on_btnOpen_clicked()
{
    if (sender) {
        delete sender;
        sender = Q_NULLPTR;
    }

    /* Select file to open */
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open Capture"),
            QDir::homePath(),
            tr("Pcap Files (*.pcap)"));
    if (fileName.isEmpty()) {
        return;
    }

    /* Update UI */
    ui->lblFilename->clear();
    ui->lblPacketCount->clear();
    ui->lblTime->clear();
    ui->progressBar->setValue(0);
    updateUIBtns();

    /* Open the capture file */
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *m_pcap_in = pcap_open_offline(fileName.toUtf8(), errbuf);
    if (!m_pcap_in) {
        error(tr("Error opening %1\n%2").arg(QDir::toNativeSeparators(fileName)).arg(errbuf));
        return;
    } else {
        ui->lblFilename->setText(QDir::toNativeSeparators(fileName));
    }

    /* Filter to only include sACN packets */
    struct bpf_program fcode;
    const char packet_filter[] = sacn_packet_filter;
    if (pcap_compile(m_pcap_in, &fcode, packet_filter, 1, 0xffffff) != 0)
    {
        error(tr("Error opening %1\npcap_compile failed").arg(QDir::toNativeSeparators(fileName)));
        return;
    }
    if (pcap_setfilter(m_pcap_in, &fcode)<0)
    {
        error(tr("Error opening %1\npcap_setfilter failed").arg(QDir::toNativeSeparators(fileName)));
        pcap_close(m_pcap_in);
    }

    /* Count packets */
    QApplication::setOverrideCursor(Qt::WaitCursor); // Might take awhile
    QApplication::processEvents();

    struct pcap_pkthdr *pkt_header;
    const u_char *pkt_data;
    unsigned int pkt_count = 0;
    QTime pkt_start_time(0,0,0), pkt_end_time(0,0,0);
    while (true)
    {
        int ret = pcap_next_ex(m_pcap_in, &pkt_header, &pkt_data);

        if (pkt_count++ == 0) {
            // First packet
            pkt_start_time = pkt_start_time.addSecs(pkt_header->ts.tv_sec);
            pkt_start_time = pkt_start_time.addMSecs((pkt_header->ts.tv_usec / 1000));
        }

        // Done!
        if (ret != 1) break;
    }

    // Get time of last packet
    pkt_end_time = pkt_end_time.addSecs(pkt_header->ts.tv_sec);
    pkt_end_time = pkt_end_time.addMSecs((pkt_header->ts.tv_usec / 1000));

    /* Clean up */
    pcap_close(m_pcap_in);

    /* Create sender thread */
    sender = new pcapplaybacksender(ui->lblFilename->text());
    connect(sender, SIGNAL(packetSent()), this, SLOT(increaseProgress()));
    connect(sender, SIGNAL(sendingFinished()), this, SLOT(playbackFinished()));
    connect(sender, SIGNAL(error(QString)), this, SLOT(error(QString)));
    ui->progressBar->setValue(0);
    sender->start();

    /* Update UI */
    ui->lblPacketCount->setText(tr("%1").arg(pkt_count));
    {
        QTime elasped(0,0,0);
        ui->lblTime->setText(QString("%1").arg(
                                 elasped.addMSecs(pkt_start_time.msecsTo(pkt_end_time)).toString("hh:mm:ss.zzz")));
    }
    ui->progressBar->setMaximum(pkt_count);
    updateUIBtns();


    QApplication::restoreOverrideCursor();
    QApplication::processEvents();
}

void PcapPlayback::on_btnStartPause_clicked()
{
    if (!sender)
    {
        sender->play();
    } else {
        if (sender->isRunning())
        {
            sender->pause();
        } else {
            sender->play();
        }
    }

    updateUIBtns();
}

void PcapPlayback::increaseProgress()
{
    ui->progressBar->setValue(ui->progressBar->value() + 1);
}

void PcapPlayback::playbackFinished()
{
    updateUIBtns();
}

void PcapPlayback::error(QString errorMessage)
{
    QMessageBox::warning(this,
        tr("PCap Playback"),
        errorMessage,
        QMessageBox::Ok);
}

void PcapPlayback::updateUIBtns()
{
    if (!sender)
    {
       ui->btnOpen->setEnabled(true);
       ui->btnStartPause->setText("Play");
       ui->btnStartPause->setEnabled(false);
       ui->btnReset->setEnabled(false);
       return;
    } else {
        ui->btnStartPause->setEnabled(true);
        ui->btnReset->setEnabled(true);
    }

    if (sender->isRunning())
    {
        ui->btnStartPause->setText("Pause");
        ui->btnOpen->setEnabled(false);
    } else {
        ui->btnStartPause->setText("Play");
        ui->btnOpen->setEnabled(true);
    }
}
