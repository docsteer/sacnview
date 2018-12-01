#include "snapshot.h"
#include "ui_snapshot.h"
#include "consts.h"
#include "preferences.h"
#include <QTimer>
#include <QSound>
#include <QSpinBox>

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

void Snapshot::on_btnAddRow_clicked()
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row + 1);

    auto universe = m_snapshots.isEmpty() ? m_firstUniverse : m_snapshots.last()->getUniverse() + 1;
    auto name = Preferences::getInstance()->GetDefaultTransmitName().append(tr(" - Snapshot"));
    clsSnapshot* snap = new clsSnapshot(universe, m_cid, name, this);

    snap->getBtnEnable()->setAutoRaise(true);
    snap->getBtnEnable()->setVisible(false);
    connect(snap, SIGNAL(senderStarted()), this, SLOT(senderStarted()));
    connect(snap, SIGNAL(senderStopped()), this, SLOT(senderStopped()));
    connect(snap, SIGNAL(senderTimedOut()), this, SLOT(senderTimedOut()));
    ui->tableWidget->setCellWidget(row, COL_BUTTON, snap->getBtnEnable());
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

           if (snap->getBtnEnable() == ui->tableWidget->cellWidget(row, COL_BUTTON))
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
    switch(s)
    {
    case stSetup:
        stopSnapshot();
        if(m_snapshots.isEmpty())
            ui->btnReplay->hide();
        else
            ui->btnReplay->show();
        ui->btnPlay->setEnabled(false);
        ui->btnSnapshot->setEnabled(ui->tableWidget->rowCount()>0);
        ui->lbTimer->hide();
        ui->btnAddRow->setEnabled(true);
        ui->btnRemoveRow->setEnabled(true);
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        ui->lbInfo->setText(tr("Add the universes you want to capture, then press Snapshot to capture a look"));
        for(auto snap: m_snapshots)
        {
            snap->getSbUniverse()->setEnabled(true);
            snap->getSbPriority()->setEnabled(true);
            snap->getBtnEnable()->setVisible(false);
            snap->getBtnEnable()->setIcon(QIcon());
        }
        break;
    case stCountDown5:
    case stCountDown4:
    case stCountDown3:
    case stCountDown2:
    case stCountDown1:
        ui->btnReplay->hide();
        ui->btnAddRow->setEnabled(false);
        ui->btnRemoveRow->setEnabled(false);
        ui->btnSnapshot->setEnabled(false);
        ui->lbInfo->setText(tr("Capturing snapshot in..."));
        ui->lbTimer->setText(QString::number(1+stCountDown1 - s));
        ui->lbTimer->show();
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        m_beep->play();
        for(auto snap: m_snapshots)
        {
            snap->getSbUniverse()->setEnabled(false);
            snap->getSbPriority()->setEnabled(false);
            snap->getBtnEnable()->setVisible(false);
            snap->getBtnEnable()->setIcon(QIcon());
        }
        break;
    case stReadyPlayback:
        ui->btnReplay->hide();
        ui->btnPlay->setEnabled(true);
        ui->btnSnapshot->setEnabled(true);
        ui->lbTimer->hide();
        ui->btnAddRow->setEnabled(true);
        ui->btnRemoveRow->setEnabled(true);
        ui->lbInfo->setText(tr("Press Play to playback snapshot"));
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        m_camera->play();
        saveSnapshot();
        for(auto snap: m_snapshots)
        {
            snap->getSbUniverse()->setEnabled(true);
            snap->getSbPriority()->setEnabled(true);
            snap->getBtnEnable()->setVisible(false);
        }
        break;
    case stReplay:
    case stPlayback:
        ui->btnReplay->hide();
        ui->btnPlay->setEnabled(true);
        ui->btnSnapshot->setEnabled(false);
        ui->lbTimer->hide();
        ui->lbInfo->setText(tr("Playing Back Data"));
        ui->btnAddRow->setEnabled(false);
        ui->btnRemoveRow->setEnabled(false);
        ui->btnPlay->setText(tr("Stop Playback"));
        ui->btnPlay->setIcon(QIcon(":/icons/pause.png"));
        playSnapshot();
        for(auto snap: m_snapshots)
        {
            snap->getSbUniverse()->setEnabled(false);
            snap->getSbPriority()->setEnabled(false);
            snap->getBtnEnable()->setVisible(true);
            snap->getBtnEnable()->setIcon(QIcon(":/icons/pause.png"));
        }
        break;
    }
    m_state = s;
}

void Snapshot::counterTick()
{
    if(m_state < stReadyPlayback)
    {
        setState((state)((int)m_state+1));
    }
    else
        m_countdown->stop();
}

void Snapshot::on_btnSnapshot_pressed()
{
    m_countdown->start(1000);
    setState(stCountDown5);
}

void Snapshot::on_btnPlay_pressed()
{
    if(m_state!=stPlayback && m_state!= stReplay)
        setState(stPlayback);
    else
        setState(stSetup);
}

void Snapshot::on_btnReplay_pressed()
{
    setState(stReplay);
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
    clsSnapshot *snap = dynamic_cast<clsSnapshot *>(sender());
    if (!snap) return;
    snap->getBtnEnable()->setIcon(QIcon(":/icons/pause.png"));
}

void Snapshot::senderStopped()
{
    clsSnapshot *snap = dynamic_cast<clsSnapshot *>(sender());
    if (!snap) return;
    snap->getBtnEnable()->setIcon(QIcon(":/icons/play.png"));
}
