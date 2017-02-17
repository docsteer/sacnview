#include "snapshot.h"
#include "ui_snapshot.h"
#include "consts.h"
#include "sacnlistener.h"
#include "sacnsender.h"
#include <QTimer>
#include <QSound>

Snapshot::Snapshot(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Snapshot), m_listener(NULL), m_sender(NULL)
{
    ui->setupUi(this);
    ui->sbUniverse->setMinimum(MIN_SACN_UNIVERSE);
    ui->sbUniverse->setMaximum(MAX_SACN_UNIVERSE);
    memset(&m_snapshotData, 0, MAX_DMX_ADDRESS);

    ui->sbPriority->setMinimum(MIN_SACN_PRIORITY);
    ui->sbPriority->setMaximum(MAX_SACN_PRIORITY);
    ui->sbPriority->setValue(DEFAULT_SACN_PRIORITY);

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

void Snapshot::setState(state s)
{
    switch(s)
    {
    case stSetup:
        stopSnapshot();
        ui->btnPlay->setEnabled(false);
        ui->btnSnapshot->setEnabled(true);
        ui->lbTimer->setText("");
        ui->sbUniverse->setEnabled(true);
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        ui->lbInfo->setText(tr("Select a universe and press Snapshot to capture a look"));
        break;
    case stCountDown5:
    case stCountDown4:
    case stCountDown3:
    case stCountDown2:
    case stCountDown1:
        ui->sbUniverse->setEnabled(false);
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
        ui->sbUniverse->setEnabled(true);
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
        ui->sbUniverse->setEnabled(false);
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
    int universe = ui->sbUniverse->value();
    sACNManager *manager = sACNManager::getInstance();
    m_listener = manager->getListener(universe);
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

void Snapshot::on_sbUniverse_valueChanged(int value)
{
    Q_UNUSED(value);
    setState(stSetup);
}

void Snapshot::saveSnapshot()
{
    if(!m_listener) return;

    for(int i=0; i<MAX_DMX_ADDRESS; i++)
    {
        m_snapshotData[i] = m_listener->mergedLevels().at(i).level;
    }
}

void Snapshot::playSnapshot()
{
    m_sender = new sACNSentUniverse(ui->sbUniverse->value());

    m_sender->setName(tr("sACNView Snapshot"));

    m_sender->startSending();
    m_sender->setLevel(m_snapshotData, MAX_DMX_ADDRESS);
}

void Snapshot::stopSnapshot()
{
    if(m_sender)
    {
        delete m_sender;
        m_sender = 0;
    }
}
