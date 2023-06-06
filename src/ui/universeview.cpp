// Copyright 2016 Tom Steer
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

#include "models/sacnsourcetablemodel.h"
#include "models/csvmodelexport.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

UniverseView::UniverseView(int universe, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UniverseView),
    m_parentWindow(parent), // needed as parent() in qobject world isn't..
    m_displayDDOnlySource(Preferences::Instance().GetETCDisplayDDOnly())
{
    ui->setupUi(this);
    UniverseDisplay *univDisplay = ui->universeDisplay;
    connect(univDisplay, &UniverseDisplay::selectedCellsChanged, this, &UniverseView::selectedAddressesChanged);
    connect(univDisplay, &UniverseDisplay::cellDoubleClick, this, &UniverseView::openBigDisplay);

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

    m_sourceTableModel = new SACNSourceTableModel(this);
    ui->tableView->setModel(m_sourceTableModel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

UniverseView::~UniverseView()
{
    delete ui;
}

void UniverseView::startListening(int universe)
{
    ui->btnGo->setEnabled(false);
    ui->btnPause->setEnabled(true);
    ui->sbUniverse->setEnabled(false);
    m_listener = sACNManager::Instance().getListener(universe);
    connect(m_listener.data(), &sACNListener::listenerStarted, this, &UniverseView::listenerStarted);
    checkBind();
    ui->universeDisplay->setUniverse(universe);

    m_sourceTableModel->clear();
    m_sourceTableModel->addListener(m_listener);

    connect(m_listener.data(), &sACNListener::sourceFound, this, &UniverseView::sourceChanged);
    connect(m_listener.data(), &sACNListener::sourceLost, this, &UniverseView::sourceChanged);
    connect(m_listener.data(), &sACNListener::sourceChanged, this, &UniverseView::sourceChanged);
    connect(m_listener.data(), &sACNListener::levelsChanged, this, &UniverseView::levelsChanged);

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

void UniverseView::sourceChanged(sACNSource*/*source*/)
{
    // Update select address details
    if (m_selectedAddress != NO_SELECTED_ADDRESS)
        selectedAddressChanged(m_selectedAddress);
}

//void UniverseView::sourceOnline(sACNSource *source)
//{
    //if (!m_listener)
    //    return;

    //// Display sources that only transmit 0xdd?
    //if (!m_displayDDOnlySource && !source->doing_dmx)
    //    return;

    //int row = ui->twSources->rowCount();
    //ui->twSources->setRowCount(row+1);
    //m_sourceToTableRow[source] = row;

    //for (auto col = 0; col < COL_END; col++)
    //    ui->twSources->setItem(row, col, new QTableWidgetItem() );

    //// Seq errors, with reset
    //{
    //    QWidget* pWidget = new QWidget();
    //    QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
    //    pLayout->setAlignment(Qt::AlignCenter);
    //    pLayout->setContentsMargins(3,0,0,0);

    //    // Count
    //    QLabel* lbl_seq = new QLabel();
    //    lbl_seq->setText(QString::number(0));
    //    pLayout->addWidget(lbl_seq);

    //    // Reset Button
    //    QPushButton* btn_seq = new QPushButton();
    //    btn_seq->setIcon(QIcon(":/icons/clear.png"));
    //    btn_seq->setFlat(true);
    //    pLayout->addWidget(btn_seq);

    //    // Connect button
    //    connect(btn_seq, &QPushButton::clicked, this, [=]() {
    //        source->resetSeqErr();
    //        this->sourceChanged(source);
    //    });

    //    // Display!
    //    pWidget->setLayout(pLayout);
    //    ui->twSources->setCellWidget(row,COL_SEQ_ERR, pWidget);
    //}

    //// Jump counter, with reset
    //{
    //    QWidget* pWidget = new QWidget();
    //    QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
    //    pLayout->setAlignment(Qt::AlignCenter);
    //    pLayout->setContentsMargins(3,0,0,0);

    //    // Count
    //    QLabel* lbl_jumps = new QLabel();
    //    lbl_jumps->setText(QString::number(0));
    //    pLayout->addWidget(lbl_jumps);

    //    // Reset Button
    //    QPushButton* btn_jumps = new QPushButton();
    //    btn_jumps->setIcon(QIcon(":/icons/clear.png"));
    //    btn_jumps->setFlat(true);
    //    pLayout->addWidget(btn_jumps);

    //    // Connect button
    //    connect(btn_jumps, &QPushButton::clicked, this, [=]() {
    //        source->resetJumps();
    //        this->sourceChanged(source);
    //    });

    //    pWidget->setLayout(pLayout);
    //    ui->twSources->setCellWidget(row,COL_JUMPS, pWidget);
    //}


//    sourceChanged(source);
//}

//void UniverseView::sourceOffline(sACNSource *source)
//{
//    if (!m_listener)
//        return;
//    sourceChanged(source);
//}

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
    int width = ui->tableView->width();

    int widthUnit = width/14;

    int used = 0;
    for (int i = SACNSourceTableModel::COL_NAME; i < SACNSourceTableModel::COL_END; ++i)
    {
        switch(i)
        {
        case SACNSourceTableModel::COL_NAME:
            break;
        case SACNSourceTableModel::COL_CID:
        case SACNSourceTableModel::COL_IP:
        case SACNSourceTableModel::COL_DD:
            ui->tableView->setColumnWidth(i, 2*widthUnit);
            used += 2*widthUnit;
            break;
        default:
            ui->tableView->setColumnWidth(i, widthUnit);
            used += widthUnit;
            break;
        }
    }

    ui->tableView->setColumnWidth(SACNSourceTableModel::COL_NAME, width-used-5);
}

QString UniverseView::prioText(const sACNSource *source, quint8 address) const
{
    if (source == nullptr)
        return tr("Unknown");

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
                    Preferences::Instance().GetFormattedValue(list[address].level),
                    prioText(list[address].winningSource, address)));

        if(list[address].otherSources.count()>0)
        {
            foreach(sACNSource *source, list[address].otherSources)
            {
                info.append(tr("\nOther Source : %1 @ %2 (Priority %3)").arg(
                            source->name,
                            Preferences::Instance().GetFormattedValue(source->level_array[address]),
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
    mainWindow->showWidgetAsSubWindow(w);
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
        if(Preferences::Instance().getFlickerFinderShowInfo())
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
    mainWindow->showWidgetAsSubWindow(w);
}

void UniverseView::on_btnExportSourceList_clicked()
{
  const QString filename = QFileDialog::getSaveFileName(this, tr("Export Sources Table"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), QStringLiteral("*.csv"));

  if (filename.isEmpty())
    return;

  // Export as CSV
  CsvModelExporter csv_export(m_sourceTableModel);
  csv_export.saveAs(filename);
}


void UniverseView::selectedAddressesChanged(QList<int> addresses)
{
    if(addresses.count()==0)
        selectedAddressChanged(NO_SELECTED_ADDRESS);
    if(addresses.count()==1)
        selectedAddressChanged(addresses.first());
}
