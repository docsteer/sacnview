#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "clssnapshot.h"
#include "consts.h"
#include "streamingacn.h"
#include <QLabel>
#include <QSoundEffect>
#include <QWidget>

namespace Ui
{
    class Snapshot;
}

class Snapshot : public QWidget
{
    Q_OBJECT
public:

    explicit Snapshot(int firstUniverse = MIN_SACN_UNIVERSE, QWidget * parent = Q_NULLPTR);
    ~Snapshot();

    enum state
    {
        stSetup,
        stCountDown5,
        stCountDown4,
        stCountDown3,
        stCountDown2,
        stCountDown1,
        stReadyPlayback,
        stPlayback,
        stReplay
    };

    class PlaybackBtn : public QToolButton
    {
    public:

        PlaybackBtn(QWidget * parent = Q_NULLPTR);

        void setPlay();
        void setPause();
    };

    class InfoLbl : public QLabel
    {
    public:

        InfoLbl(QWidget * parent = Q_NULLPTR);

        void setText(Snapshot::state state);
        void setText(const QString & t) { QLabel::setText(t); }
    };

protected slots:
    void counterTick();
    void on_btnSnapshot_pressed();
    void on_btnPlay_pressed();
    void btnPlay_update(bool updateState = false);
    void on_btnAddRow_clicked();
    void on_btnAddRow_rightClicked();
    void on_btnRemoveRow_clicked();
    void senderTimedOut();
    void senderStopped();
    void senderStarted();
    void updateMatchIcon();

protected:

    virtual void resizeEvent(QResizeEvent * event);

private slots:
    void on_tableWidget_itemSelectionChanged();

private:

    enum
    {
        COL_BUTTON,
        COL_UNIVERSE,
        COL_PRIORITY,
        COLCOUNT
    };

    void addUniverse(quint16 universe);

    void setState(state s);
    void setState(int s) { setState(static_cast<state>(s)); }
    void saveSnapshot();
    void playSnapshot();
    void stopSnapshot();
    Ui::Snapshot * ui;
    QTimer * m_countdown;
    state m_state;
    QSoundEffect *m_camera, *m_beep;
    QList<clsSnapshot *> m_snapshots;
    quint16 m_firstUniverse;
    CID m_cid;
};

#endif // SNAPSHOT_H
