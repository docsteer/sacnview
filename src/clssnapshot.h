#ifndef CLSSNAPSHOT_H
#define CLSSNAPSHOT_H

#include <QObject>
#include <QSpinBox>
#include <QToolButton>
#include <QMap>
#include <QSound>
#include "streamingacn.h"
#include "sacnlistener.h"
#include "sacnsender.h"

class clsSnapshot : public QWidget
{
    Q_OBJECT
public:
    explicit clsSnapshot(quint16 universe, CID cid, QString name, QWidget *parent = nullptr);
    ~clsSnapshot();

    void takeSnapshot();
    void playSnapshot();
    void stopSnapshot();

    bool hasData() { return !m_levelData.isEmpty(); }

    bool isPlaying() { return m_sender->isSending(); }

    quint16 getUniverse() {return m_universe;}
    void setUniverse(quint16 universe);

    quint8 getPriority() {return m_priority;}
    void setPriority(quint8 priority);

    QSpinBox *getSbUniverse() {return m_sbUniverse;}
    QSpinBox *getSbPriority() {return m_sbPriority;}
    QToolButton *getBtnPlayback() {return m_btnPlayback;}

    enum e_icons {
        ICON_NONE,
        ICON_PLAY,
        ICON_PAUSE,
        ICON_SNAPSHOT
    };

    const QMap<e_icons, QIcon> icons{
        {ICON_NONE, QIcon()},
        {ICON_PLAY, QIcon(":/icons/play.png")},
        {ICON_PAUSE, QIcon(":/icons/pause.png")},
        {ICON_SNAPSHOT, QIcon(":/icons/snapshot.png")}
    };

signals:
    void senderStarted();
    void senderStopped();
    void senderTimedOut();
    void snapshotTaken();

public slots:

private slots:
    void btnEnableClicked(bool value);

private:
    quint16 m_universe;
    quint8 m_priority;
    CID m_cid;

    QByteArray m_levelData;
    QSpinBox* m_sbUniverse;
    QSpinBox* m_sbPriority;
    QToolButton* m_btnPlayback;
    sACNManager::tSender m_sender;
    sACNManager::tListener m_listener;

    QSound *m_camera;
};

#endif // CLSSNAPSHOT_H
