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
#include "bigdisplay.h"
#include "consts.h"
#include "flickerfinderinfoform.h"
#include "logwindow.h"
#include "mdimainwindow.h"
#include "preferences.h"
#include "sacnlistener.h"
#include "streamingacn.h"
#include "ui_universeview.h"

#include "models/csvmodelexport.h"
#include "models/sacnsourcetablemodel.h"

#include "delegates/resettablecounterdelegate.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

UniverseView::UniverseView(int universe, QWidget * parent)
    : QWidget(parent)
    , ui(new Ui::UniverseView)
    , m_parentWindow(parent)
    , // needed as parent() in qobject world isn't..
    m_displayDDOnlySource(Preferences::Instance().GetETCDisplayDDOnly())
{
    ui->setupUi(this);
    UniverseDisplay * univDisplay = ui->universeDisplay;
    connect(univDisplay, &UniverseDisplay::selectedCellsChanged, this, &UniverseView::selectedAddressesChanged);
    connect(univDisplay, &UniverseDisplay::cellDoubleClick, this, &UniverseView::openBigDisplay);

    connect(ui->btnShowPriority, &QPushButton::toggled, univDisplay, &UniverseDisplay::setShowChannelPriority);
    connect(univDisplay, &UniverseDisplay::showChannelPriorityChanged, ui->btnShowPriority, &QPushButton::setChecked);

    connect(univDisplay, &UniverseDisplay::universeChanged, this, &UniverseView::refreshTitle);
    connect(univDisplay, &UniverseDisplay::flickerFinderChanged, this, &UniverseView::onFlickerFinderChanged);
    connect(univDisplay, &UniverseDisplay::compareToUniverseChanged, this, &UniverseView::onCompareUniverseChanged);

    ui->sbUniverse->setRange(MIN_SACN_UNIVERSE, MAX_SACN_UNIVERSE);
    ui->sbUniverse->setWrapping(true);
    ui->sbUniverse->setEnabled(true);
    ui->sbUniverse->setValue(universe);

    ui->sbCompareUniverse->setRange(MIN_SACN_UNIVERSE, MAX_SACN_UNIVERSE);
    ui->sbCompareUniverse->setWrapping(true);
    ui->sbCompareUniverse->setEnabled(true);
    ui->sbCompareUniverse->setValue(universe + 1);

    m_sourceTableModel = new SACNSourceTableModel(this);
    QSortFilterProxyModel * sortProxy = new QSortFilterProxyModel(this);
    sortProxy->setSourceModel(m_sourceTableModel);
    ui->tableView->setModel(sortProxy);
    ui->tableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    // Don't need to display the Universe column
    ui->tableView->setColumnHidden(SACNSourceTableModel::COL_UNIVERSE, true);
    // Don't show the timing detail columns
    for (int col : SACNSourceTableModel::TimingDetailColumns) ui->tableView->setColumnHidden(col, true);
    // Maybe don't show the Secure column
    ui->tableView->setColumnHidden(
        SACNSourceTableModel::COL_PATHWAY_SECURE,
        !Preferences::Instance().GetPathwaySecureRx());
    // Don'display the Notes column
    ui->tableView->setColumnHidden(SACNSourceTableModel::COL_NOTES, true);

    // Set the delegates for resettable counters
    ui->tableView->setItemDelegateForColumn(SACNSourceTableModel::COL_JUMPS, new ResettableCounterDelegate());
    ui->tableView->setItemDelegateForColumn(SACNSourceTableModel::COL_SEQ_ERR, new ResettableCounterDelegate());

    // Allow the user to temporarily rearrange the columns
    ui->tableView->horizontalHeader()->setSectionsMovable(true);

    // Not running
    updateButtons(false);
}

UniverseView::~UniverseView()
{
    delete ui;
}

void UniverseView::startListening(int universe)
{
    updateButtons(true);

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

    if (ui->sbUniverse->value() != universe) ui->sbUniverse->setValue(universe);
    if (ui->sbCompareUniverse->value() == universe) ui->sbCompareUniverse->setValue(universe + 1);
}

void UniverseView::refreshTitle()
{
    if (!m_listener)
    {
        setWindowTitle(tr("Universe View"));
        return;
    }

    const int universe = m_listener->universe();
    const int compareToUniverse = ui->universeDisplay->getCompareToUniverse();

    if (ui->universeDisplay->getFlickerFinder())
        setWindowTitle(tr("Universe %1 Flicker").arg(universe));
    else if (compareToUniverse != UniverseDisplay::NO_UNIVERSE)
        setWindowTitle(tr("Comparing Universe %1 to %2").arg(universe).arg(compareToUniverse));
    else
        setWindowTitle(tr("Universe %1 View").arg(universe));
}

QJsonObject UniverseView::getJsonConfiguration() const
{
    QJsonObject result;
    result[QLatin1String("universe")] = ui->sbUniverse->value();
    result[QLatin1String("priorities")] = ui->universeDisplay->showChannelPriority();
    result[QLatin1String("compare_universe")] = ui->sbCompareUniverse->value();
    return result;
}

void UniverseView::setJsonConfiguration(const QJsonObject & json)
{
    ui->sbUniverse->setValue(json[QLatin1String("universe")].toInt(ui->sbUniverse->value()));
    ui->universeDisplay->setShowChannelPriority(json[QLatin1String("priorities")].toBool());
    ui->sbCompareUniverse->setValue(json[QLatin1String("compare_universe")].toInt(ui->sbCompareUniverse->value()));
}

void UniverseView::on_btnGo_clicked()
{
    startListening(ui->sbUniverse->value());
}

void UniverseView::checkBind()
{
    bool bindOk(m_listener->getBindStatus().multicast != sACNRxSocket::BIND_FAILED);
    bindOk &= m_listener->getBindStatus().unicast != sACNRxSocket::BIND_FAILED;

    if (bindOk || m_bindWarningShown) return;

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(
        tr("Errors binding to interface\r\n\r\nResults will be inaccurate\r\nPossible reasons include permission "
           "issues\r\nor other applications\r\n\r\nSee diagnostics for more info"));
    m_bindWarningShown = true;
    msgBox.exec();
}

void UniverseView::updateButtons(bool running)
{
    // Enable/disable buttons
    ui->btnGo->setEnabled(!running);
    ui->btnPause->setEnabled(running);
    ui->sbUniverse->setEnabled(!running);

    ui->btnStartFlickerFinder->setEnabled(running);
    ui->btnCompareUniverse->setEnabled(running);
}

void UniverseView::listenerStarted(int universe)
{
    Q_UNUSED(universe);

    checkBind();
}

void UniverseView::onFlickerFinderChanged()
{
    if (ui->universeDisplay->getFlickerFinder())
        ui->btnStartFlickerFinder->setText(tr("Stop Flicker Finder"));
    else
        ui->btnStartFlickerFinder->setText(tr("Start Flicker Finder"));

    refreshTitle();
}

void UniverseView::onCompareUniverseChanged()
{
    if (ui->universeDisplay->getCompareToUniverse() != UniverseDisplay::NO_UNIVERSE)
        ui->btnCompareUniverse->setText(tr("Stop Delta Finder"));
    else
        ui->btnCompareUniverse->setText(tr("Start Delta Finder"));

    refreshTitle();
}

void UniverseView::sourceChanged(sACNSource * /*source*/)
{
    // Update select address details
    if (m_selectedAddress != NO_SELECTED_ADDRESS) selectedAddressChanged(m_selectedAddress);
}

void UniverseView::levelsChanged()
{
    if (!m_listener) return;
    if (m_selectedAddress != NO_SELECTED_ADDRESS) selectedAddressChanged(m_selectedAddress);
}

void UniverseView::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);
    resizeColumns();
}

void UniverseView::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);

    resizeColumns();
}

void UniverseView::resizeColumns()
{
    // Attempt to resize so all columns fit
    int width = ui->tableView->width();

    int widthUnit = width / 14;

    int used = 0;
    for (int i = SACNSourceTableModel::COL_NAME; i < SACNSourceTableModel::COL_END; ++i)
    {
        switch (i)
        {
            case SACNSourceTableModel::COL_NAME: break;
            case SACNSourceTableModel::COL_CID:
            case SACNSourceTableModel::COL_IP:
            case SACNSourceTableModel::COL_DD:
                ui->tableView->setColumnWidth(i, 2 * widthUnit);
                used += 2 * widthUnit;
                break;
            default:
                ui->tableView->setColumnWidth(i, widthUnit);
                used += widthUnit;
                break;
        }
    }

    ui->tableView->setColumnWidth(SACNSourceTableModel::COL_NAME, width - used - 5);
}

QString UniverseView::prioText(const sACNSource * source, quint8 address) const
{
    if (source == nullptr) return tr("Unknown");

    if (source->doing_per_channel)
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
    if (address < 0) return;

    if (!m_listener) return;
    sACNMergedSourceList list = m_listener->mergedLevels();

    QString info;

    info.append(tr("Address : %1\n").arg(address + 1));

    if (list[address].winningSource)
    {
        info.append(tr("Winning Source : %1 @ %2 (Priority %3)")
                        .arg(
                            list[address].winningSource->name,
                            Preferences::Instance().GetFormattedValue(list[address].level),
                            prioText(list[address].winningSource, address)));

        if (list[address].otherSources.count() > 0)
        {
            foreach (sACNSource * source, list[address].otherSources)
            {
                info.append(tr("\nOther Source : %1 @ %2 (Priority %3)")
                                .arg(
                                    source->name,
                                    Preferences::Instance().GetFormattedValue(source->level_array[address]),
                                    prioText(source, address)));
            }
        }
    }
    else
    {
        if (list[address].winningPriority <= 0)
        {
            info.append(tr("Unpatched"));
        }
        else
        {
            info.append(tr("No Sources"));
        }
    }

    ui->teInfo->setPlainText(info);
}

void UniverseView::openBigDisplay(quint16 address)
{
    MDIMainWindow * mainWindow = dynamic_cast<MDIMainWindow *>(m_parentWindow);
    if (!mainWindow) return;
    BigDisplay * w = new BigDisplay(ui->sbUniverse->value(), address, mainWindow);
    mainWindow->showWidgetAsSubWindow(w);
}

void UniverseView::on_btnPause_clicked()
{
    ui->universeDisplay->pause();
    m_sourceTableModel->pause();

    this->disconnect(m_listener.data());

    updateButtons(false);

    m_bindWarningShown = false;

    setWindowTitle(tr("Universe View"));
}

void UniverseView::on_btnStartFlickerFinder_clicked()
{
    if (ui->universeDisplay->getFlickerFinder())
    {
        ui->universeDisplay->setFlickerFinder(false);
    }
    else
    {
        if (Preferences::Instance().getFlickerFinderShowInfo())
        {
            FlickerFinderInfoForm form;
            int result = form.exec();
            if (!result) return;
        }
        ui->universeDisplay->setFlickerFinder(true);
    }
}

void UniverseView::on_btnCompareUniverse_clicked()
{
    if (ui->universeDisplay->getCompareToUniverse() == UniverseDisplay::NO_UNIVERSE)
    {
        ui->universeDisplay->setCompareToUniverse(ui->sbCompareUniverse->value());
    }
    else
    {
        ui->universeDisplay->setCompareToUniverse(UniverseDisplay::NO_UNIVERSE);
    }
}

void UniverseView::on_sbCompareUniverse_editingFinished()
{
    int val = ui->sbCompareUniverse->value();

    // Can't compare to same
    const int univ = ui->sbUniverse->value();
    if (val == univ)
    {
        val = univ + 1;
        ui->sbCompareUniverse->setValue(val);
    }

    // Change if active and different
    const int currentCompare = ui->universeDisplay->getCompareToUniverse();
    if (currentCompare != UniverseDisplay::NO_UNIVERSE && currentCompare != val)
    {
        ui->universeDisplay->setCompareToUniverse(val);
    }
}

void UniverseView::on_btnClearOffline_clicked()
{
    if (m_sourceTableModel) m_sourceTableModel->clearOffline();
}

void UniverseView::on_btnLogWindow_clicked()
{
    MDIMainWindow * mainWindow = qobject_cast<MDIMainWindow *>(m_parentWindow);
    if (!mainWindow) return;
    LogWindow * w = new LogWindow(ui->sbUniverse->value(), mainWindow);
    mainWindow->showWidgetAsSubWindow(w);
}

void UniverseView::on_btnExportSourceList_clicked()
{
    const QString filename = QFileDialog::getSaveFileName(
        this,
        tr("Export Sources Table"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QStringLiteral("*.csv"));

    if (filename.isEmpty()) return;

    // Export as CSV
    CsvModelExporter csv_export(m_sourceTableModel);
    csv_export.saveAs(filename);
}

void UniverseView::selectedAddressesChanged(QList<int> addresses)
{
    if (addresses.count() == 0) selectedAddressChanged(NO_SELECTED_ADDRESS);
    if (addresses.count() == 1) selectedAddressChanged(addresses.first());
}
