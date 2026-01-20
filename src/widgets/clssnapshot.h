#ifndef CLSSNAPSHOT_H
#define CLSSNAPSHOT_H

#include "sacnlistener.h"
#include "sacnsender.h"
#include "streamingacn.h"
#include <QLabel>
#include <QMap>
#include <QObject>
#include <QSoundEffect>
#include <QSpinBox>
#include <QToolButton>

class clsSnapshot : public QWidget
{
    Q_OBJECT
public:

    explicit clsSnapshot(quint16 universe, CID cid, QString name, QWidget * parent = nullptr);
    ~clsSnapshot();

    void takeSnapshot();
    void playSnapshot();
    void stopSnapshot();

    bool hasData() { return !m_levelData.isEmpty(); }

    bool isPlaying() { return (m_sender ? m_sender->isSending() : false); }

    quint16 getUniverse() { return m_universe; }
    void setUniverse(quint16 universe);

    quint8 getPriority() { return m_priority; }
    void setPriority(quint8 priority);

    QSpinBox * getSbUniverse() { return m_sbUniverse; }
    QSpinBox * getSbPriority() { return m_sbPriority; }
    QWidget * getControlWidget() { return m_controlWidget; }

    bool isMatching() const { return m_backgroundMatches; }

    enum e_icons
    {
        ICON_NONE,
        ICON_PLAY,
        ICON_PAUSE,
        ICON_SNAPSHOT,
    };

    const QMap<e_icons, QIcon> icons{
        {ICON_NONE, QIcon()},
        {ICON_PLAY, QIcon(":/icons/play.png")},
        {ICON_PAUSE, QIcon(":/icons/pause.png")},
        {ICON_SNAPSHOT, QIcon(":/icons/snapshot.png")}};

    enum e_statusIcons
    {
        STATUSICON_NONE,
        STATUSICON_MATCHING,
        STATUSICON_NOTMATCHING
    };

    const QMap<e_statusIcons, QPixmap> statusIcons{
        {STATUSICON_NONE, QPixmap()},
        {STATUSICON_MATCHING, QPixmap(":/icons/ledgreen.png")},
        {STATUSICON_NOTMATCHING, QPixmap(":/icons/ledred.png")}};

    const QMap<e_statusIcons, QString> statusIconTooltips{
        {STATUSICON_NONE, QString()},
        {STATUSICON_MATCHING, tr("The snapshot <i>matches</i> the other sources in this universe")},
        {STATUSICON_NOTMATCHING, tr("The snapshot <i>does not match</i> the other sources in this universe")}};

signals:
    void senderStarted();
    void senderStopped();
    void senderTimedOut();
    void snapshotTaken();
    void snapshotMatches();
    void snapshotDiffers();

public slots:

private slots:
    void btnEnableClicked(bool value);
    void levelsChanged();

private:

    void updateIcons();

    quint16 m_universe;
    quint8 m_priority;
    CID m_cid;

    QByteArray m_levelData;
    QSpinBox * m_sbUniverse;
    QSpinBox * m_sbPriority;
    QToolButton * m_btnPlayback;
    QLabel * m_lblStatus;
    QWidget * m_controlWidget;
    sACNManager::tSender m_sender;
    sACNManager::tListener m_listener;

    QSoundEffect * m_camera;

    bool m_backgroundMatches;
};

#endif // CLSSNAPSHOT_H
