#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <QWidget>
#include <QtMultimedia/QSound>
#include <QSpinBox>
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
    void on_btnAddRow_pressed();
    void on_btnRemoveRow_pressed();
protected:
    virtual void resizeEvent(QResizeEvent *event);
private:
    void setState(state s);
    void saveSnapshot();
    void playSnapshot();
    void stopSnapshot();
    Ui::Snapshot *ui;
    QTimer *m_countdown;
    state m_state;
    QSound *m_camera, *m_beep;
    QList<QByteArray> m_snapshotData;
    QList<QSharedPointer<sACNListener>>m_listeners;
    QList<sACNSentUniverse *>m_senders;
    QList<QSpinBox *> m_universeSpins;
    QList<QSpinBox *> m_prioritySpins;
};

#endif // SNAPSHOT_H
