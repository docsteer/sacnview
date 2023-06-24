#include "pcapplayback.h"
#include "ui_pcapplayback.h"
#include "pcapplaybacksender.h"
#include "pcap.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QThread>
#include <QDebug>

#ifdef Q_OS_WIN
#include <QLibrary>
#endif
bool PcapPlayback::foundLib()
{
#ifdef Q_OS_WIN
    QLibrary lib("wpcap");
    return lib.load();
#else
    // It's statically linked!
    return true;
    #endif
}

PcapPlayback::PcapPlayback(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PcapPlayback),
    sender(NULL)
{
    ui->setupUi(this);
}

PcapPlayback::~PcapPlayback()
{
    closeThread();
    delete ui;
}

void PcapPlayback::show()
{
    if (foundLib())
    {
        QWidget::show();
    } else {
        #ifdef Q_OS_WIN
        const auto npcap = QStringLiteral("<a href='https://npcap.com/'>npcap</a>");
        const auto winpcap = QStringLiteral("<a href='https://www.winpcap.org/'>winpcap</a>");
        const auto pcap = QString("%1 or %2").arg(npcap, winpcap);
        #else
        const auto pcap = QStringLiteral("<a href='https://www.tcpdump.org/'>libpcap</a>");
        #endif

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(QString("pcap not found!<br>Please install: %1").arg(pcap));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

        this->close();
        this->deleteLater();
    }
}

void PcapPlayback::openThread()
{
    if (sender) {
        closeThread();
    }

    /* Create sender thread */
    sender = new pcapplaybacksender(ui->lblFilename->text());
    connect(sender, SIGNAL(packetSent()), this, SLOT(increaseProgress()));
    connect(sender, SIGNAL(sendingFinished()), this, SLOT(playbackFinished()));
    connect(sender, SIGNAL(sendingClosed()), this, SLOT(playbackClosed()));
    connect(sender, SIGNAL(error(QString)), this, SLOT(error(QString)));
    connect(sender, &QThread::finished, [this]() {
        sender->deleteLater();
        sender = Q_NULLPTR; });
    connect(sender, SIGNAL(finished()), this, SLOT(playbackThreadClosed()));
    ui->progressBar->reset();
    sender->start();
    sender->setPriority(QThread::HighPriority);
}

void PcapPlayback::closeThread()
{
    if (!sender)
        return;

    sender->quit();
    sender = Q_NULLPTR;
}

void PcapPlayback::playbackThreadClosed()
{
    updateUIBtns();
}

void PcapPlayback::on_btnOpen_clicked()
{
    /* Select file to open */
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open Capture"),
            QDir::homePath(),
            tr("PCap Files (*.pcap);; PCapNG Files (*.pcapng);; All files (*.*)"));
    if (fileName.isEmpty()) {
        return;
    }

    /* Update UI */
    updateUIBtns(true);

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
        if (ret != 1) break;

        if (pkt_count++ == 0) {
            // First packet
            pkt_start_time = pkt_start_time.addSecs(pkt_header->ts.tv_sec);
            pkt_start_time = pkt_start_time.addMSecs((pkt_header->ts.tv_usec / 1000));
        }

    }

    // Get time of last packet
    pkt_end_time = pkt_end_time.addSecs(pkt_header->ts.tv_sec);
    pkt_end_time = pkt_end_time.addMSecs((pkt_header->ts.tv_usec / 1000));

    /* Clean up */
    pcap_close(m_pcap_in);

    /* Open Thread */
    openThread();

    /* Update UI */
    ui->lblFileCount->setText(QString("%1").arg(pkt_count));
    ui->progressBar->setMaximum(pkt_count);
    {
        QTime elasped(0,0,0);
        ui->lblFileTime->setText(QString("%1").arg(
                                 elasped.addMSecs(pkt_start_time.msecsTo(pkt_end_time)).toString("hh:mm:ss.zzz")));
    }
    updateUIBtns();


    QApplication::restoreOverrideCursor();
    QApplication::processEvents();
}

void PcapPlayback::on_btnStartPause_clicked()
{
    if (sender)
    {
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
    if (ui->chkLoop->isChecked())
    {
        on_btnReset_clicked();
        sender->play();
    }
    updateUIBtns();
}

void PcapPlayback::playbackClosed()
{
    updateUIBtns();
}

void PcapPlayback::error(QString errorMessage)
{
    QMessageBox::warning(this,
        tr("PCap Playback"),
        errorMessage,
        QMessageBox::Ok);

    closeThread();

    updateUIBtns(true);
}

void PcapPlayback::updateUIBtns(bool clear)
{
    if (clear)
    {
        ui->lblFilename->clear();
        ui->lblFileCount->clear();
        ui->lblFileTime->clear();
        ui->progressBar->reset();
    }

    if (!sender)
    {
       ui->btnOpen->setEnabled(true);
       ui->btnStartPause->setText("Play");
       ui->btnStartPause->setEnabled(false);
       ui->btnReset->setEnabled(false);
       ui->chkLoop->setEnabled(false);
       return;
    } else {
        ui->btnStartPause->setEnabled(true);
        ui->btnReset->setEnabled(true);
        ui->chkLoop->setEnabled(true);
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

void PcapPlayback::on_btnReset_clicked()
{
    updateUIBtns();
    ui->progressBar->reset();
    sender->reset();
}
