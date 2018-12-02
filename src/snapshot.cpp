#include "snapshot.h"
#include "ui_snapshot.h"
#include "qt56.h"
#include "consts.h"
#include "preferences.h"
#include <QTimer>
#include <QSound>
#include <QSpinBox>

Snapshot::PlaybackBtn::PlaybackBtn(QWidget *parent) : QToolButton(parent)
{
    setPlay();
}

void Snapshot::PlaybackBtn::setPlay()
{
    this->setIcon(QIcon(":/icons/play.png"));
    this->setText(tr("Playback All Snapshots"));
}

void Snapshot::PlaybackBtn::setPause()
{
    this->setIcon(QIcon(":/icons/pause.png"));
    this->setText(tr("Stop All Playback"));
}

Snapshot::InfoLbl::InfoLbl(QWidget *parent) : QLabel(parent)
{
    setText(stSetup);
}

void Snapshot::InfoLbl::setText(Snapshot::state state)
{
    switch (state)
    {
        case stSetup:
            setText(tr("Add the universes you want to capture, then press Snapshot to capture a look"));
            break;
        case stCountDown5:
            Q_FALLTHROUGH();
        case stCountDown4:
            Q_FALLTHROUGH();
        case stCountDown3:
            Q_FALLTHROUGH();
        case stCountDown2:
            Q_FALLTHROUGH();
        case stCountDown1:
            setText(tr("Capturing snapshot in..."));
            break;
        case stReadyPlayback:
            Q_FALLTHROUGH();
        case stReplay:
            setText(tr("Press Play to playback snapshot"));
            break;
        case stPlayback:
            setText(tr("Replaying Data"));
            break;
    }
}

Snapshot::Snapshot(int firstUniverse, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Snapshot),
    m_countdown(new QTimer(this)),
    m_camera(new QSound(":/sound/camera.wav", this)),
    m_beep(new QSound(":/sound/beep.wav", this)),
    m_firstUniverse(firstUniverse)
{
    ui->setupUi(this);

    m_cid = CID::CreateCid();

    connect(m_countdown, SIGNAL(timeout()), this, SLOT(counterTick()));

    setState(stSetup);
}

Snapshot::~Snapshot()
{
    stopSnapshot();
    delete ui;
}


Snapshot::state& operator++(Snapshot::state& s) // Prefix
{
    switch (s)
    {
        case Snapshot::state::stCountDown5: return s = Snapshot::state::stCountDown4;
        case Snapshot::state::stCountDown4: return s = Snapshot::state::stCountDown3;
        case Snapshot::state::stCountDown3: return s = Snapshot::state::stCountDown2;
        case Snapshot::state::stCountDown2: return s = Snapshot::state::stCountDown1;
        case Snapshot::state::stCountDown1: return s = Snapshot::state::stReadyPlayback;
        default: return s;
    }
}

void Snapshot::on_btnAddRow_clicked()
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row + 1);

    auto universe = m_snapshots.isEmpty() ? m_firstUniverse : m_snapshots.last()->getUniverse() + 1;
    auto name = Preferences::getInstance()->GetDefaultTransmitName().append(tr(" - Snapshot"));
    clsSnapshot* snap = new clsSnapshot(universe, m_cid, name, this);

    snap->getBtnPlayback()->setAutoRaise(true);
    connect(snap, SIGNAL(senderStarted()), this, SLOT(senderStarted()));
    connect(snap, SIGNAL(senderStopped()), this, SLOT(senderStopped()));
    connect(snap, SIGNAL(senderTimedOut()), this, SLOT(senderTimedOut()));
    connect(snap, &clsSnapshot::snapshotTaken, [this]() { ui->btnPlay->setEnabled(true); });
    connect(snap, SIGNAL(snapshotMatches()), this, SLOT(snapshotMatches()));
    connect(snap, SIGNAL(snapshotDiffers()), this, SLOT(snapshotDiffers()));
    ui->tableWidget->setCellWidget(row, COL_BUTTON, snap->getBtnPlayback());
    ui->tableWidget->setCellWidget(row, COL_UNIVERSE, snap->getSbUniverse());
    ui->tableWidget->setCellWidget(row, COL_PRIORITY, snap->getSbPriority());

    setState(stSetup);

    m_snapshots.append(snap);
}

void Snapshot::on_btnRemoveRow_clicked()
{
    // Get selected rows
    QList<int> rows;
    for(auto selection: ui->tableWidget->selectionModel()->selectedRows())
    {
        rows << selection.row();
    }

    // Sort backwards
    std::sort(rows.rbegin(), rows.rend());

    // Remove from bottom
    for(auto row: rows)
    {
        for(auto snap: m_snapshots)
        {

           if (snap->getBtnPlayback() == ui->tableWidget->cellWidget(row, COL_BUTTON))
           {
               ui->tableWidget->removeRow(row);
               m_snapshots.removeOne(snap);
               snap->deleteLater();
               break;
           }
        }
    }
    ui->tableWidget->clearSelection();
    ui->btnSnapshot->setEnabled(ui->tableWidget->rowCount()>0);
    setState(stSetup);
}

void Snapshot::setState(state s)
{
    // Info Label Text
    ui->lbInfo->setText(s);

    // Play Button Text and Icon
    btnPlay_update();

    switch(s)
    {
    case stSetup:
        ui->btnPlay->setEnabled(false);
        ui->btnSnapshot->setEnabled(ui->tableWidget->rowCount()>0);
        ui->lbTimer->hide();
        ui->btnAddRow->setEnabled(true);
        on_tableWidget_itemSelectionChanged();

        for(auto snap: m_snapshots)
        {
            snap->getSbUniverse()->setEnabled(true);
            snap->getSbPriority()->setEnabled(true);
            if (snap->hasData())
                ui->btnPlay->setEnabled(true);
        }
        break;
    case stCountDown5:
        Q_FALLTHROUGH();
    case stCountDown4:
        Q_FALLTHROUGH();
    case stCountDown3:
        Q_FALLTHROUGH();
    case stCountDown2:
        Q_FALLTHROUGH();
    case stCountDown1:
        ui->btnAddRow->setEnabled(false);
        ui->btnRemoveRow->setEnabled(false);
        ui->btnSnapshot->setEnabled(false);
        ui->lbTimer->setText(QString::number(1+stCountDown1 - s));
        ui->lbTimer->show();
        m_beep->play();
        for(auto snap: m_snapshots)
        {
            snap->getSbUniverse()->setEnabled(false);
            snap->getSbPriority()->setEnabled(false);
        }
        m_countdown->start(1000);
        break;
    case stReadyPlayback:
        m_camera->play();
        saveSnapshot();
        Q_FALLTHROUGH();
    case stReplay:
        ui->btnPlay->setEnabled(true);
        ui->btnSnapshot->setEnabled(true);
        ui->lbTimer->hide();
        ui->btnAddRow->setEnabled(true);
        on_tableWidget_itemSelectionChanged();
        break;
    case stPlayback:
        ui->btnPlay->setEnabled(true);
        ui->btnSnapshot->setEnabled(false);
        ui->lbTimer->hide();
        ui->btnAddRow->setEnabled(false);
        ui->btnRemoveRow->setEnabled(false);
        for(auto snap: m_snapshots)
        {
            snap->getSbUniverse()->setEnabled(false);
            snap->getSbPriority()->setEnabled(false);
        }
        break;
    }
    m_state = s;
}

void Snapshot::counterTick()
{
    if(m_state != stReadyPlayback)
        setState(++m_state);
    else
        m_countdown->stop();
}

void Snapshot::on_btnSnapshot_pressed()
{
    setState(stCountDown5);
}

void Snapshot::btnPlay_update(bool updateState)
{
    bool displayPause = false;
    for(auto snap: m_snapshots)
        if (snap->isPlaying())
            displayPause = true;
    if (displayPause)
    {
        ui->btnPlay->setPause();
        if (updateState && m_state != stPlayback)
            setState(stPlayback);
    }
    else
    {
        ui->btnPlay->setPlay();
        if (updateState && m_state != stReplay)
            setState(stReplay);
    }
}

void Snapshot::on_btnPlay_pressed()
{
    if(m_state!=stPlayback)
    {
        playSnapshot();
        setState(stPlayback);
    }
    else
    {
        stopSnapshot();
        setState(stReplay);
    }
}

void Snapshot::saveSnapshot()
{
    for(auto snap: m_snapshots)
        snap->takeSnapshot();
}

void Snapshot::playSnapshot()
{
    for(auto snap: m_snapshots)
        snap->playSnapshot();
}

void Snapshot::stopSnapshot()
{
    for(auto snap: m_snapshots)
        snap->stopSnapshot();
}

void Snapshot::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int width = (ui->tableWidget->width() - 30) / 2;
    ui->tableWidget->setColumnWidth(0, 30);
    ui->tableWidget->setColumnWidth(1, width - 3);
}

void Snapshot::senderTimedOut()
{
    senderStopped();
}

void Snapshot::senderStarted()
{
//    clsSnapshot *snap = dynamic_cast<clsSnapshot *>(sender());
//    if (!snap) return;
    btnPlay_update(true);
}

void Snapshot::senderStopped()
{
//    clsSnapshot *snap = dynamic_cast<clsSnapshot *>(sender());
//    if (!snap) return;
    btnPlay_update(true);
}

void Snapshot::on_tableWidget_itemSelectionChanged()
{
    if (
            (m_state == stReadyPlayback) ||
            (m_state == stReplay) ||
            (m_state == stSetup)
        )
        ui->btnRemoveRow->setEnabled(!ui->tableWidget->selectionModel()->selectedRows().isEmpty());
    else
       ui->btnRemoveRow->setEnabled(false);
}
