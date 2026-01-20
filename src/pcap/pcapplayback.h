#ifndef PCAPPLAYBACK_H
#define PCAPPLAYBACK_H

#include "pcapplaybacksender.h"
#include <QWidget>

namespace Ui
{
    class PcapPlayback;
}

class PcapPlayback : public QWidget
{
    Q_OBJECT

public:

    explicit PcapPlayback(QWidget * parent = 0);
    ~PcapPlayback();

    static bool foundLibPcap();
    static std::string getLibPcapVersion();

private slots:
    void on_btnOpen_clicked();
    void on_btnStartPause_clicked();
    void increaseProgress();
    void playbackFinished();
    void playbackClosed();
    void playbackThreadClosed();
    void error(QString errorMessage);

    void on_btnReset_clicked();

private:

    Ui::PcapPlayback * ui;

    pcapplaybacksender * sender;

    void openThread();
    void closeThread();
    void updateUIBtns(bool clear = false);
};

#endif // PCAPPLAYBACK_H
