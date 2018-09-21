#include "snapshot.h"
#include "ui_snapshot.h"
#include "consts.h"
#include "sacnlistener.h"
#include "sacnsender.h"
#include "preferences.h"
#include <QTimer>
#include <QSound>
#include <QSpinBox>

Snapshot::Snapshot(int firstUniverse, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Snapshot),
    m_listeners(QList<sACNManager::tListener>()),
    m_senders(QList<sACNManager::tSender>()),
    m_firstUniverse(firstUniverse)
{
    ui->setupUi(this);

    m_cid = CID::CreateCid();

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

    QToolButton *btnStartStop = new QToolButton(this);
    btnStartStop->setAutoRaise(true);
    m_enableButtons << btnStartStop;
    ui->tableWidget->setCellWidget(row, COL_BUTTON, btnStartStop);
    btnStartStop->setVisible(false);
    connect(btnStartStop, SIGNAL(pressed()), this, SLOT(pauseSourceButtonPressed()));

    QSpinBox *sbUniverse = new QSpinBox(this);
    sbUniverse->setMinimum(MIN_SACN_UNIVERSE);
    sbUniverse->setMaximum(MAX_SACN_UNIVERSE);
    if(m_universeSpins.count()>0)
        sbUniverse->setValue(m_universeSpins.last()->value()+1);
    else
        sbUniverse->setValue(m_firstUniverse);

    ui->tableWidget->setCellWidget(row, COL_UNIVERSE, sbUniverse);
    m_universeSpins << sbUniverse;

    QSpinBox *sbPriority = new QSpinBox(this);
    sbPriority->setMinimum(MIN_SACN_PRIORITY);
    sbPriority->setMaximum(MAX_SACN_PRIORITY);
    sbPriority->setValue(DEFAULT_SACN_PRIORITY);
    ui->tableWidget->setCellWidget(row, COL_PRIORITY, sbPriority);
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
    m_enableButtons.removeAt(row);
}

void Snapshot::setState(state s)
{
    switch(s)
    {
    case stSetup:
        stopSnapshot();
        if(m_snapshotData.isEmpty())
            ui->btnReplay->hide();
        else
            ui->btnReplay->show();
        ui->btnPlay->setEnabled(false);
        ui->btnSnapshot->setEnabled(ui->tableWidget->rowCount()>0);
        ui->lbTimer->setText("");
        ui->btnAddRow->setEnabled(true);
        ui->btnRemoveRow->setEnabled(true);
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        ui->lbInfo->setText(tr("Add the universes you want to capture, then press Snapshot to capture a look"));
        foreach(QSpinBox *s, m_universeSpins)
            s->setEnabled(true);
        foreach(QSpinBox *s, m_prioritySpins)
            s->setEnabled(true);
        foreach(QToolButton *t, m_enableButtons)
        {
            t->setVisible(false);
            t->setIcon(QIcon());
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
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        m_beep->play();
        foreach(QSpinBox *s, m_universeSpins)
            s->setEnabled(false);
        foreach(QSpinBox *s, m_prioritySpins)
            s->setEnabled(false);
        foreach(QToolButton *t, m_enableButtons)
            t->setVisible(false);
        break;
    case stReadyPlayback:
        ui->btnReplay->hide();
        ui->btnPlay->setEnabled(true);
        ui->btnSnapshot->setEnabled(true);
        ui->lbTimer->setText("");
        ui->btnAddRow->setEnabled(true);
        ui->btnRemoveRow->setEnabled(true);
        ui->lbInfo->setText(tr("Press Play to playback snapshot"));
        ui->btnPlay->setText(tr("Play Back Snapshot"));
        ui->btnPlay->setIcon(QIcon(":/icons/play.png"));
        m_camera->play();
        saveSnapshot();
        foreach(QSpinBox *s, m_universeSpins)
            s->setEnabled(true);
        foreach(QSpinBox *s, m_prioritySpins)
            s->setEnabled(true);
        foreach(QToolButton *t, m_enableButtons)
            t->setVisible(false);
        break;
    case stReplay:
    case stPlayback:
        ui->btnReplay->hide();
        ui->btnPlay->setEnabled(true);
        ui->btnSnapshot->setEnabled(false);
        ui->lbTimer->setText("");
        ui->lbInfo->setText(tr("Playing Back Data"));
        ui->btnAddRow->setEnabled(false);
        ui->btnRemoveRow->setEnabled(false);
        ui->btnPlay->setText(tr("Stop Playback"));
        ui->btnPlay->setIcon(QIcon(":/icons/pause.png"));
        playSnapshot();
        foreach(QSpinBox *s, m_universeSpins)
            s->setEnabled(false);
        foreach(QSpinBox *s, m_prioritySpins)
            s->setEnabled(false);
        foreach(QToolButton *t, m_enableButtons)
        {
            t->setVisible(true);
            t->setIcon(QIcon(":/icons/pause.png"));
        }
        break;
    }
    m_state = s;

    foreach(QToolButton *t, m_enableButtons)
        qDebug() << "Toolbutton vis " << t->isVisible();

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
    m_snapshotData.clear();

    for(int i=0; i<m_listeners.count(); i++)
    {
        QByteArray b;
        for(int j=0; j<MAX_DMX_ADDRESS; j++)
        {
            int level = m_listeners[i]->mergedLevels().at(j).level;

            if(level>0) {
#if QT_VERSION >= 0x050700
                b.append(1, (char) level);
#else
                b.append(QByteArray().fill((char)level, 1));
#endif
            }
            else {
#if QT_VERSION >= 0x050700
                b.append(1, 0);
#else
                b.append(QByteArray().fill(0, 1));
#endif
            }
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
        m_senders << sACNManager::getInstance()->getSender(m_universeSpins[i]->value(), m_cid);

        {
            QString name = Preferences::getInstance()->GetDefaultTransmitName();
            QString postfix = tr(" - Snapshot");
            name.truncate(MAX_SOURCE_NAME_LEN - postfix.length());
            m_senders.last().data()->setName(name.trimmed() + postfix);
            m_senders.last().data()->setPerSourcePriority(m_prioritySpins[i]->value());
        }

        m_senders.last().data()->startSending();
        m_senders.last().data()->setLevel((const quint8*)m_snapshotData[i].constData(), MAX_DMX_ADDRESS);
        connect(m_senders.last().data(), SIGNAL(sendingTimeout()), this, SLOT(senderTimedOut()));
    }
}

void Snapshot::stopSnapshot()
{
    for(int i=0; i<m_senders.count(); i++)
            m_senders[i].data()->deleteLater();
    m_senders.clear();
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
    setState(stSetup);
}

void Snapshot::pauseSourceButtonPressed()
{
    QToolButton *btn = dynamic_cast<QToolButton *>(sender());
    if(!btn) return;
    qDebug() << "Btn Vis is " << btn->isVisible();
    int index = m_enableButtons.indexOf(btn);
    if(index<0 || index >= m_senders.count()) return;

    sACNSentUniverse *sender = m_senders[index].data();

    if(sender->isSending())
    {
        // Stop it
        btn->setIcon(QIcon(":/icons/play.png"));
        sender->stopSending();
    }
    else
    {
        btn->setIcon(QIcon(":/icons/pause.png"));
        sender->startSending();
    }
}
