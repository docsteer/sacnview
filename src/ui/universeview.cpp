// Copyright 2016 Tom Barthel-Steer
// http://www.tomsteer.net
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "universeview.h"
#include "ui_universeview.h"
#include "streamingacn.h"
#include "sacnlistener.h"
#include "preferences.h"
#include "consts.h"
#include "flickerfinderinfoform.h"
#include "logwindow.h"
#include "mdimainwindow.h"
#include "bigdisplay.h"

#include <QMessageBox>

QString protocolVerToString(int value)
{
    switch(value)
    {
    case sACNProtocolDraft:
        return QObject::tr("Draft");
    case sACNProtocolRelease:
        return QObject::tr("Release");
    }

    return QObject::tr("Unknown");
}

UniverseView::UniverseView(int universe, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UniverseView),
    m_parentWindow(parent), // needed as parent() in qobject world isn't..
    m_displayDDOnlySource(Preferences::getInstance()->GetDisplayDDOnly())
{
    ui->setupUi(this);
    UniverseDisplay *univDisplay = ui->universeDisplay;
    connect(univDisplay, SIGNAL(selectedCellsChanged(QList<int>)), this, SLOT(selectedAddressesChanged(QList<int>)));
    connect(univDisplay, SIGNAL(cellDoubleClick(quint16)), this, SLOT(openBigDisplay(quint16)));

    connect(ui->btnShowPriority, &QPushButton::toggled, univDisplay, &UniverseDisplay::setShowChannelPriority);
    connect(univDisplay, &UniverseDisplay::showChannelPriorityChanged, ui->btnShowPriority, &QPushButton::setChecked);

    connect(univDisplay, &UniverseDisplay::universeChanged, this, &UniverseView::refreshTitle);
    connect(univDisplay, &UniverseDisplay::flickerFinderChanged, this, &UniverseView::refreshTitle);

    ui->btnGo->setEnabled(true);
    ui->btnPause->setEnabled(false);
    ui->sbUniverse->setMinimum(MIN_SACN_UNIVERSE);
    ui->sbUniverse->setMaximum(MAX_SACN_UNIVERSE);
    ui->sbUniverse->setWrapping(true);
    ui->sbUniverse->setEnabled(true);
    ui->sbUniverse->setValue(universe);

    ui->twSources->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

UniverseView::~UniverseView()
{
    delete ui;
}

void UniverseView::startListening(int universe)
{
    m_sourceToTableRow.clear();
    ui->twSources->setRowCount(0);
    ui->btnGo->setEnabled(false);
    ui->btnPause->setEnabled(true);
    ui->sbUniverse->setEnabled(false);
    m_listener = sACNManager::getInstance()->getListener(universe);
    connect(m_listener.data(), SIGNAL(listenerStarted(int)), this, SLOT(listenerStarted(int)));
    checkBind();
    ui->universeDisplay->setUniverse(universe);

    // Add the existing sources
    for(int i=0; i<m_listener->sourceCount(); i++)
    {
        sourceOnline(m_listener->source(i));
    }
    connect(m_listener.data(), SIGNAL(sourceFound(sACNSource*)), this, SLOT(sourceOnline(sACNSource*)));
    connect(m_listener.data(), SIGNAL(sourceLost(sACNSource*)), this, SLOT(sourceOffline(sACNSource*)));
    connect(m_listener.data(), SIGNAL(sourceChanged(sACNSource*)), this, SLOT(sourceChanged(sACNSource*)));
    connect(m_listener.data(), SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));

    if(ui->sbUniverse->value()!=universe)
        ui->sbUniverse->setValue(universe);
}

void UniverseView::refreshTitle()
{
    if (!m_listener)
    {
        setWindowTitle(tr("Universe View"));
        return;
    }

    int universe = m_listener->universe();
    bool flicker = ui->universeDisplay->getFlickerFinder();

    if (flicker)
        setWindowTitle(tr("Universe %1 Flicker").arg(universe));
    else
        setWindowTitle(tr("Universe %1 View").arg(universe));
}

void UniverseView::on_btnGo_clicked()
{
    startListening(ui->sbUniverse->value());
}

void UniverseView::checkBind()
{
    bool bindOk(m_listener->getBindStatus().multicast != sACNRxSocket::BIND_FAILED);
    bindOk &= m_listener->getBindStatus().unicast != sACNRxSocket::BIND_FAILED;

    if (bindOk || m_bindWarningShown)
        return;

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(tr("Errors binding to interface\r\n\r\nResults will be inaccurate\r\nPossible reasons include permission issues\r\nor other applications\r\n\r\nSee diagnostics for more info"));
    m_bindWarningShown = true;
    msgBox.exec();
}

void UniverseView::listenerStarted(int universe)
{
    Q_UNUSED(universe);

    checkBind();
}

void UniverseView::sourceChanged(sACNSource *source)
{
    if (!m_listener)
        return;

    if (!m_sourceToTableRow.contains(source))
    {
        return;
    }

    // Display sources that only transmit 0xdd?
    if (!m_displayDDOnlySource && !source->doing_dmx) { return; }

    int row = m_sourceToTableRow[source];
    ui->twSources->item(row,COL_NAME)->setText(source->name);
    ui->twSources->item(row,COL_NAME)->setBackground(Preferences::getInstance()->colorForCID(source->src_cid));
    ui->twSources->item(row,COL_CID)->setText(source->src_cid);
    ui->twSources->item(row,COL_PRIO)->setText(QString::number(source->priority));

    if (source->protocol_version == sACNProtocolDraft)
        ui->twSources->item(row,COL_PREVIEW)->setText(tr("N/A"));
    else
        ui->twSources->item(row,COL_PREVIEW)->setText(source->isPreview ? tr("Yes") : tr("No"));

    if (source->protocol_version == sACNProtocolDraft)
        ui->twSources->item(row,COL_SYNC)->setText(tr("N/A"));
    else
        ui->twSources->item(row,COL_SYNC)->setText(source->synchronization
            ? QString(tr("Yes (%1)")).arg(source->synchronization)
            : tr("No"));

    ui->twSources->item(row,COL_IP)->setText(source->ip.toString());
    ui->twSources->item(row,COL_FPS)->setText(QString("%1Hz").arg(QString::number(source->fpscounter.FPS(), 'f', 2)));
    {
        // Seq Errors
        QLabel* lbl_SEQ_ERR = qobject_cast<QLabel *>(
                    ui->twSources->cellWidget(row,COL_SEQ_ERR)->layout()->itemAt(0)->widget());
        lbl_SEQ_ERR->setText(QString::number(source->seqErr));
    }
    {
        // Jumps
        QLabel* lbl_SEQ_ERR = qobject_cast<QLabel *>(
                    ui->twSources->cellWidget(row,COL_JUMPS)->layout()->itemAt(0)->widget());
        lbl_SEQ_ERR->setText(QString::number(source->jumps));
    }

    if (source->src_valid) {
        if (source->doing_dmx) {
            if (source->src_stable) {
                ui->twSources->item(row,COL_ONLINE)->setBackground(Qt::green);
                ui->twSources->item(row,COL_ONLINE)->setText(tr("Online"));
            } else {
                ui->twSources->item(row,COL_ONLINE)->setBackground(Qt::yellow);
                ui->twSources->item(row,COL_ONLINE)->setText(tr("Online (Unstable)"));
            }
        } else {
            ui->twSources->item(row,COL_ONLINE)->setBackground(Qt::yellow);
            ui->twSources->item(row,COL_ONLINE)->setText(tr("No DMX"));
        }
    } else {
        ui->twSources->item(row,COL_ONLINE)->setBackground(Qt::red);
        ui->twSources->item(row,COL_ONLINE)->setText(tr("Offline"));
    }

    ui->twSources->item(row,COL_VER)->setText(protocolVerToString(source->protocol_version));
    ui->twSources->item(row,COL_DD)->setText(
                source->doing_per_channel ?
                    (Preferences::getInstance()->GetIgnoreDD() ? tr("Ignored") : tr("Yes"))
                    : tr("No"));
    ui->twSources->item(row,COL_SLOTS)->setText(QString::number(source->slot_count));

    // Update select address details
    if (m_selectedAddress != NO_SELECTED_ADDRESS)
        selectedAddressChanged(m_selectedAddress);
}

void UniverseView::sourceOnline(sACNSource *source)
{
    if (!m_listener)
        return;

    // Display sources that only transmit 0xdd?
    if (!m_displayDDOnlySource && !source->doing_dmx) { return; }

    int row = ui->twSources->rowCount();
    ui->twSources->setRowCount(row+1);
    m_sourceToTableRow[source] = row;

    for (auto col = 0; col < COL_END; col++)
        ui->twSources->setItem(row, col, new QTableWidgetItem() );

    // Seq errors, with reset
    {
        QWidget* pWidget = new QWidget();
        QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
        pLayout->setAlignment(Qt::AlignCenter);
        pLayout->setContentsMargins(3,0,0,0);

        // Count
        QLabel* lbl_seq = new QLabel();
        lbl_seq->setText(QString::number(0));
        pLayout->addWidget(lbl_seq);

        // Reset Button
        QPushButton* btn_seq = new QPushButton();
        btn_seq->setIcon(QIcon(":/icons/clear.png"));
        btn_seq->setFlat(true);
        pLayout->addWidget(btn_seq);

        // Connect button
        connect(btn_seq, &QPushButton::clicked, this, [=]() {
            source->resetSeqErr();
            this->sourceChanged(source);
        });

        // Display!
        pWidget->setLayout(pLayout);
        ui->twSources->setCellWidget(row,COL_SEQ_ERR, pWidget);
    }

    // Jump counter, with reset
    {
        QWidget* pWidget = new QWidget();
        QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
        pLayout->setAlignment(Qt::AlignCenter);
        pLayout->setContentsMargins(3,0,0,0);

        // Count
        QLabel* lbl_jumps = new QLabel();
        lbl_jumps->setText(QString::number(0));
        pLayout->addWidget(lbl_jumps);

        // Reset Button
        QPushButton* btn_jumps = new QPushButton();
        btn_jumps->setIcon(QIcon(":/icons/clear.png"));
        btn_jumps->setFlat(true);
        pLayout->addWidget(btn_jumps);

        // Connect button
        connect(btn_jumps, &QPushButton::clicked, this, [=]() {
            source->resetJumps();
            this->sourceChanged(source);
        });

        pWidget->setLayout(pLayout);
        ui->twSources->setCellWidget(row,COL_JUMPS, pWidget);
    }


    sourceChanged(source);
}

void UniverseView::sourceOffline(sACNSource *source)
{
    if (!m_listener)
        return;
    sourceChanged(source);
}

void UniverseView::levelsChanged()
{
    if (!m_listener)
        return;
    if (m_selectedAddress != NO_SELECTED_ADDRESS)
        selectedAddressChanged(m_selectedAddress);
}

void UniverseView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    resizeColumns();
}

void UniverseView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    resizeColumns();
}

void UniverseView::resizeColumns()
{
    // Attempt to resize so all columns fit
    int width = ui->twSources->width();

    int widthUnit = width/14;

    int used = 0;
    for (int i = COL_NAME; i < COL_END; ++i)
    {
        switch(i)
        {
        case COL_NAME:
            break;
        case COL_CID:
        case COL_IP:
        case COL_DD:
            ui->twSources->setColumnWidth(i, 2*widthUnit);
            used += 2*widthUnit;
            break;
        default:
            ui->twSources->setColumnWidth(i, widthUnit);
            used += widthUnit;
            break;
        }
    }

    ui->twSources->setColumnWidth(COL_NAME, width-used-5);
}

QString UniverseView::prioText(sACNSource *source, quint8 address)
{
    if (source == nullptr)
        return "Unknown";

    if(source->doing_per_channel)
        if (source->priority_array[address] > 0)
            return QString::number(source->priority_array[address]);
        else
            return tr("Unpatched");
    else
        return QString::number(source->priority);
}

void UniverseView::selectedAddressChanged(int address)
{
    ui->teInfo->clear();
    m_selectedAddress = address;
    ui->teInfo->clear();
    if(address<0) return;

    if(!m_listener) return;
    sACNMergedSourceList list = m_listener->mergedLevels();

    QString info;

    info.append(tr("Address : %1\n")
                .arg(address+1));

    if(list[address].winningSource)
    {
        info.append(tr("Winning Source : %1 @ %2 (Priority %3)").arg(
                    list[address].winningSource->name,
                    Preferences::getInstance()->GetFormattedValue(list[address].level),
                    prioText(list[address].winningSource, address)));

        if(list[address].otherSources.count()>0)
        {
            foreach(sACNSource *source, list[address].otherSources)
            {
                info.append(tr("\nOther Source : %1 @ %2 (Priority %3)").arg(
                            source->name,
                            Preferences::getInstance()->GetFormattedValue(source->level_array[address]),
                            prioText(source, address)));
            }
        }
    } else {
        if (list[address].winningPriority <= 0)
        {
            info.append(tr("Unpatched"));
        } else {
            info.append(tr("No Sources"));
        }
    }

    ui->teInfo->setPlainText(info);
}

void UniverseView::openBigDisplay(quint16 address)
{
    MDIMainWindow *mainWindow = dynamic_cast<MDIMainWindow *>(m_parentWindow);
    if(!mainWindow) return;
    BigDisplay *w = new BigDisplay(ui->sbUniverse->value(), address, mainWindow);
    mainWindow->showWidgetAsMdiWindow(w);
}

void UniverseView::on_btnPause_clicked()
{
    ui->universeDisplay->pause();
    this->disconnect(m_listener.data());
    ui->btnGo->setEnabled(true);
    ui->btnPause->setEnabled(false);
    ui->sbUniverse->setEnabled(true);
    m_bindWarningShown = false;

    setWindowTitle(tr("Universe View"));
}

void UniverseView::on_btnStartFlickerFinder_clicked()
{
    if(ui->universeDisplay->getFlickerFinder())
    {
        ui->universeDisplay->setFlickerFinder(false);
        ui->btnStartFlickerFinder->setText(tr("Start Flicker Finder"));
    }
    else
    {
        if(Preferences::getInstance()->getFlickerFinderShowInfo())
        {
            FlickerFinderInfoForm form;
            int result = form.exec();
            if(!result) return;
        }
        ui->universeDisplay->setFlickerFinder(true);
        ui->btnStartFlickerFinder->setText(tr("Stop Flicker Finder"));
    }
}


void UniverseView::on_btnLogWindow_clicked()
{
    MDIMainWindow *mainWindow = qobject_cast<MDIMainWindow *>(m_parentWindow);
    if (!mainWindow)
        return;
    LogWindow *w = new LogWindow(ui->sbUniverse->value(),mainWindow);
    mainWindow->showWidgetAsMdiWindow(w);
}


void UniverseView::selectedAddressesChanged(QList<int> addresses)
{
    if(addresses.count()==0)
        selectedAddressChanged(NO_SELECTED_ADDRESS);
    if(addresses.count()==1)
        selectedAddressChanged(addresses.first());
}
