#include "snapshot.h"
#include "ui_snapshot.h"
#include "consts.h"
#include "preferences.h"
#include <QTimer>
#include <QSoundEffect>
#include <QSpinBox>
#include <QInputDialog>
#include <QMessageBox>

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
    m_camera(new QSoundEffect(this)),
    m_beep(new QSoundEffect(this)),
    m_firstUniverse(firstUniverse)
{
    ui->setupUi(this);

    m_cid = CID::CreateCid();

    m_camera->setSource(QUrl("qrc:/sound/camera.wav"));
    m_beep->setSource(QUrl("qrc:/sound/beep.wav"));

    connect(m_countdown, &QTimer::timeout, this, &Snapshot::counterTick);

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
    auto universe = m_snapshots.isEmpty() ? m_firstUniverse : m_snapshots.last()->getUniverse() + 1;
    addUniverse(universe);
}

void Snapshot::on_btnAddRow_rightClicked()
{
    bool ok;
    QString rawInput = QInputDialog::getText(this,
                    tr("Capture universes"),
                    tr("List of capture universes"), QLineEdit::Normal,
                    "1,2,3,4-10", &ok);

    if (!ok)
        return;

    // Decode the string range of universes
    auto strList = rawInput.split(",", Qt::SkipEmptyParts);
    QList<quint16> universeList;
    for (const auto& item : std::as_const(strList)) {
        if (item.contains("-")) {
            // Ranges
            auto strRange = item.split("-", Qt::SkipEmptyParts);
            auto first = strRange.first().toUInt(&ok);
            if (!ok) continue;
            auto last = strRange.last().toUInt(&ok);
            if (!ok) continue;
            if (first > last)
                std::swap(first, last);
            for (unsigned int universe = first; universe <= last; ++universe) {
                if (universe >= MIN_SACN_UNIVERSE && universe <= MAX_SACN_UNIVERSE)
                    if (!universeList.contains(universe))
                        universeList << universe;
            }
        } else {
            // Singles
            auto universe = item.toUInt(&ok);
            if (ok && universe >= MIN_SACN_UNIVERSE && universe <= MAX_SACN_UNIVERSE)
                if (!universeList.contains(universe))
                    universeList << universe;
        }
    }
    std::sort(universeList.begin(), universeList.end());

    qDebug() << "Snapshot add list raw" << rawInput;
    qDebug() << "Snapshot add list parsed" << universeList;

    if (universeList.count() >= 100) {
        auto warningReply =
                QMessageBox::warning(this,
                    tr("Warning"),
                    tr("Adding a large number of universes\r\n"
                       "may cause instability and take a long time\r\n"
                       "\r\n"
                       "Continue?"),
                    QMessageBox::Yes|QMessageBox::No);
        if (warningReply == QMessageBox::No)
            return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (const auto universe : std::as_const(universeList)) {
        addUniverse(universe);
        QCoreApplication::processEvents();
    }
    QApplication::restoreOverrideCursor();
}

void Snapshot::on_btnRemoveRow_clicked()
{
    // Get selected rows
    QList<int> rows;
    for(const auto &selection: ui->tableWidget->selectionModel()->selectedRows())
    {
        rows << selection.row();
    }

    // Sort backwards
    std::sort(rows.rbegin(), rows.rend());

    // Remove from bottom
    for(const auto &row: std::as_const(rows))
    {
        for(const auto snap: std::as_const(m_snapshots))
        {

           if (snap->getControlWidget() == ui->tableWidget->cellWidget(row, COL_BUTTON))
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

void Snapshot::addUniverse(quint16 universe)
{
    if (universe < MIN_SACN_UNIVERSE || universe > MAX_SACN_UNIVERSE)
        return;

    // Check it's not a duplicate
    for (const auto &snapshot : std::as_const(m_snapshots)) {
        if (snapshot->getUniverse() == universe)
            return;
    }

    // Add new snapshot item
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row + 1);

    QString name = Preferences::Instance().GetDefaultTransmitName() + tr(" - Snapshot");
    clsSnapshot* snap = new clsSnapshot(universe, m_cid, name, this);

    connect(snap, &clsSnapshot::senderStarted, this, &Snapshot::senderStarted);
    connect(snap, &clsSnapshot::senderStopped, this, &Snapshot::senderStopped);
    connect(snap, &clsSnapshot::senderTimedOut, this, &Snapshot::senderTimedOut);
    connect(snap, &clsSnapshot::snapshotTaken, this, [this]() { ui->btnPlay->setEnabled(true); });
    connect(snap, &clsSnapshot::snapshotMatches, this, &Snapshot::updateMatchIcon);
    connect(snap, &clsSnapshot::snapshotDiffers, this, &Snapshot::updateMatchIcon);
    ui->tableWidget->setCellWidget(row, COL_BUTTON, snap->getControlWidget());
    ui->tableWidget->setCellWidget(row, COL_UNIVERSE, snap->getSbUniverse());
    ui->tableWidget->setCellWidget(row, COL_PRIORITY, snap->getSbPriority());

    setState(stSetup);

    m_snapshots.append(snap);
}

void Snapshot::setState(state s)
{
    // Info Label Text
    ui->lbInfo->setText(s);

    // Play Button Text and Icon
    btnPlay_update();

    updateMatchIcon();

    switch(s)
    {
    case stSetup:
        ui->btnPlay->setEnabled(false);
        ui->btnSnapshot->setEnabled(ui->tableWidget->rowCount()>0);
        ui->lbTimer->hide();
        ui->btnAddRow->setEnabled(true);
        on_tableWidget_itemSelectionChanged();
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
    for(const auto snap: std::as_const(m_snapshots))
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
    for(const auto snap: std::as_const(m_snapshots))
        snap->takeSnapshot();
}

void Snapshot::playSnapshot()
{
    for(const auto snap: std::as_const(m_snapshots))
        snap->playSnapshot();
}

void Snapshot::stopSnapshot()
{
    for(const auto snap: std::as_const(m_snapshots))
        snap->stopSnapshot();
}

void Snapshot::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int width = (ui->tableWidget->width() - 60) / 2;
    ui->tableWidget->setColumnWidth(0, 60);
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

void Snapshot::updateMatchIcon()
{
    if(m_state == stPlayback)
    {
        bool allmatch = true;
        for(const auto snap: std::as_const(m_snapshots))
            allmatch &= snap->isMatching();
        if(allmatch)
        {
            ui->lblMatching->setPixmap(QPixmap(":/icons/ledgreen.png"));
            ui->lblMatching->setToolTip(tr("All sources <i>match</i> the background levels"));
        }
        else
        {
            ui->lblMatching->setPixmap(QPixmap(":/icons/ledred.png"));
            ui->lblMatching->setToolTip(tr("Not all sources match the background levels"));
        }
    }
    else {
        ui->lblMatching->setPixmap(QPixmap());
    }
}
