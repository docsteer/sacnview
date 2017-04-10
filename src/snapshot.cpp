#include "snapshot.h"
#include "ui_snapshot.h"
#include "consts.h"
#include "sacnlistener.h"
#include "sacnsender.h"
#include <QTimer>
#include <QSound>
#include <QSpinBox>

Snapshot::Snapshot(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Snapshot),
    m_listeners(QList<QSharedPointer<sACNListener>>()),
    m_senders(QList<sACNSentUniverse *>())
{
    ui->setupUi(this);

    m_countdown = new QTimer(this);
    connect(m_countdown, SIGNAL(timeout()), this, SLOT(counterTick()));

    m_camera = new QSound(":/sound/camera.wav", this);
    m_beep = new QSound(":/sound/beep.wav", this);

    setState(stSetup);
}

Snapshot::~Snapshot()
{
    stopSnapshot();
    delete ui;
}

void Snapshot::on_btnAddRow_pressed()
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row + 1);

    QSpinBox *sbUniverse = new QSpinBox(this);
    sbUniverse->setMinimum(MIN_SACN_UNIVERSE);
    sbUniverse->setMaximum(MAX_SACN_UNIVERSE);
    if(m_universeSpins.count()>0)
        sbUniverse->setValue(m_universeSpins.last()->value()+1);

    ui->tableWidget->setCellWidget(row, 0, sbUniverse);
    m_universeSpins << sbUniverse;

    QSpinBox *sbPriority = new QSpinBox(this);
    sbPriority->setMinimum(MIN_SACN_PRIORITY);
    sbPriority->setMaximum(MAX_SACN_PRIORITY);
    sbPriority->setValue(DEFAULT_SACN_PRIORITY);
    ui->tableWidget->setCellWidget(row, 1, sbPriority);
    m_prioritySpins << sbPriority;

    ui->btnSnapshot->setEnabled(ui->tableWidget->rowCount()>0);
    setState(stSetup);
}

void Snapshot::on_btnRemoveRow_pressed()
{
    int row = ui->tableWidget->currentRow();
    ui->tableWidget->removeRow(row);
    ui->btnSnapshot->setEnabled(ui->tableWidget->rowCount()>0);
    setState(stSetup);
    m_universeSpins.removeAt(row);
    m_prioritySpins.removeAt(row);
}

void Snapshot::setState(state s)
{
    switch(s)
    {
    case stSetup:
        stopSnapshot();
        ui->btnPlay->setEnabled(false);
        ui->btnSnapshot->setEnabled(ui->tableWidget->rowCount()>0);
        ui->lbTimer->setText("");
        ui->tableWidget->setEnabled(true);
        ui->btnAddRow->setEnabled(true);
        ui->btnRemoveRow->setEnabled(true);
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        ui->lbInfo->setText(tr("Add the universes you want to capture, then press Snapshot to capture a look"));
        break;
    case stCountDown5:
    case stCountDown4:
    case stCountDown3:
    case stCountDown2:
    case stCountDown1:
        ui->tableWidget->setEnabled(false);
        ui->btnAddRow->setEnabled(false);
        ui->btnRemoveRow->setEnabled(false);
        ui->btnSnapshot->setEnabled(false);
        ui->lbInfo->setText(tr("Capturing snapshot in..."));
        ui->lbTimer->setText(QString::number(1+stCountDown1 - s));
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        m_beep->play();
        break;
    case stReadyPlayback:
        ui->btnPlay->setEnabled(true);
        ui->btnSnapshot->setEnabled(true);
        ui->lbTimer->setText("");
        ui->tableWidget->setEnabled(true);
        ui->btnAddRow->setEnabled(true);
        ui->btnRemoveRow->setEnabled(true);
        ui->lbInfo->setText(tr("Press Play to playback snapshot"));
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        m_camera->play();
        saveSnapshot();
        break;
    case stPlayback:
        ui->btnPlay->setEnabled(true);
        ui->btnSnapshot->setEnabled(false);
        ui->lbTimer->setText("");
        ui->lbInfo->setText(tr("Playing Back Data"));
        ui->tableWidget->setEnabled(false);
        ui->btnAddRow->setEnabled(false);
        ui->btnRemoveRow->setEnabled(false);
        ui->btnPlay->setText(tr("Stop Playback"));
        ui->btnPlay->setIcon(QIcon(":/icons/pause.png"));
        playSnapshot();
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
    for(int i=0; i<ui->tableWidget->rowCount(); i++)
    {
        int universe = m_universeSpins[i]->value();

        sACNManager *manager = sACNManager::getInstance();
        m_listeners << manager->getListener(universe);
    }
    m_countdown->start(1000);
    setState(stCountDown5);
}

void Snapshot::on_btnPlay_pressed()
{
    if(m_state!=stPlayback)
        setState(stPlayback);
    else
        setState(stSetup);
}

void Snapshot::saveSnapshot()
{
    m_snapshotData.clear();

    for(int i=0; i<m_listeners.count(); i++)
    {
        QByteArray b;
        for(int j=0; j<MAX_DMX_ADDRESS; j++)
        {
            int level = m_listeners[i]->mergedLevels().at(j).level;
            if(level>0)
                b.append(1, (char) level);
            else
                b.append(1, 0);
        }
        m_snapshotData << b;
    }

    // Free up the listeners
    m_listeners.clear();

}

void Snapshot::playSnapshot()
{
    for(int i=0; i<m_snapshotData.count(); i++)
    {
        m_senders << new sACNSentUniverse(m_universeSpins[i]->value());

        m_senders.last()->setName(tr("sACNView Snapshot"));

        m_senders.last()->startSending();
        m_senders.last()->setLevel((const quint8*)m_snapshotData[i].constData(), MAX_DMX_ADDRESS);
    }
}

void Snapshot::stopSnapshot()
{
    if(m_senders.count()>0)
    {
        qDeleteAll(m_senders);
        m_senders.clear();
    }
}

void Snapshot::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int width = ui->tableWidget->width() / 2;
    ui->tableWidget->setColumnWidth(0, width - 3);
}
