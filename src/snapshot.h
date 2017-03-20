#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <QWidget>
#include <QtMultimedia/QSound>
#include "streamingacn.h"
#include "consts.h"

namespace Ui {
class Snapshot;
}

class Snapshot : public QWidget
{
    Q_OBJECT

    enum state
    {
        stSetup,
        stCountDown5,
        stCountDown4,
        stCountDown3,
        stCountDown2,
        stCountDown1,
        stReadyPlayback,
        stPlayback
    };

public:
    explicit Snapshot(QWidget *parent = 0);
    ~Snapshot();
protected slots:
    void counterTick();
    void on_btnSnapshot_pressed();
    void on_btnPlay_pressed();
    void on_sbUniverse_valueChanged(int value);
private:
    void setState(state s);
    void saveSnapshot();
    void playSnapshot();
    void stopSnapshot();
    Ui::Snapshot *ui;
    QTimer *m_countdown;
    state m_state;
    QSound *m_camera, *m_beep;
    quint8 m_snapshotData[MAX_DMX_ADDRESS];
    QSharedPointer<sACNListener>m_listener;
    sACNSentUniverse *m_sender;
};

#endif // SNAPSHOT_H
