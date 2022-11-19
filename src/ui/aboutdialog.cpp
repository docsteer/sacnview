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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "consts.h"
#include <pcap.h>
#include "translations/translations.h"

#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include <QStringList>

aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutDialog)
{
    ui->setupUi(this);

    if (QString(VERSION) == QString(GIT_CURRENT_SHA1)) {
        ui->DisplayVer->setText(QString("%1").arg(VERSION));
    } else {
        ui->DisplayVer->setText(QString("%1\n%2").arg(VERSION).arg(GIT_CURRENT_SHA1));
    }
    ui->displayDate->setText(QString("%1, %2 %3 %4").arg(GIT_DATE_DAY).arg(GIT_DATE_DATE).arg(GIT_DATE_MONTH).arg(GIT_DATE_YEAR));
    ui->DisplayName->setText(AUTHORS.join("\n"));

    // Translators
    QStringList translators;
    for (auto translations : Translations::lTranslations)
    {
        for (auto translatorName : translations.Translators)
        {
            if (!translators.contains(translatorName))
                translators << translatorName;
        }
    }
    ui->lblTranslatorNames->setText(translators.join("\n"));

    // Libs
    ui->lblLicense->setText(
                tr("This application is provided under the <a href=\"http://www.apache.org/licenses/LICENSE-2.0\">Apache License, version 2.0</a>")
    );
    ui->lblQtInfo->setText(tr("This application uses the Qt Library, version %1, licensed under the <a href=\"http://www.gnu.org/licenses/lgpl.html\">GNU LGPL</a>")
                           .arg(qVersion()));
    
    const char *libpcap = pcap_lib_version();
    ui->lblLibs->setText(ui->lblLibs->text() + tr("This application uses the pcap Library, version %1, licensed under the <a href=\"https://opensource.org/licenses/BSD-3-Clause\">The 3-Clause BSD License</a>")
            .arg(libpcap));
    ui->lblLibs->setText(ui->lblLibs->text() + "\n");
    
    ui->lblLibs->setText(ui->lblLibs->text() + tr("This application uses BLAKE2, licensed under the <a href=\"https://creativecommons.org/publicdomain/zero/1.0/\">Creative Commons Zero v1.0 Universal</a>"));
    ui->lblLibs->setText(ui->lblLibs->text() + "\n");

    // Setup diagnostics tree
    ui->twDiag->setColumnCount(2);
    ui->twDiag->header()->hide();

    // Get listener list
    const auto listenerList = sACNManager::getInstance()->getListenerList();

    // Sort list
    struct {
            bool operator()(const QWeakPointer<sACNListener> &a, const QWeakPointer<sACNListener> &b) const
            {
                auto aStrong = a.toStrongRef();
                auto bStrong = b.toStrongRef();
                if (aStrong && bStrong)
                    return aStrong->universe() < bStrong->universe();
                else if (!aStrong)
                    return true;
                else
                    return false;
            }
        } listenerSortbyUni;
    auto listenerListSorted = listenerList.values();
    std::sort(listenerListSorted.begin(), listenerListSorted.end(), listenerSortbyUni);

    // Display Sorted!
    for (auto weakListener : listenerListSorted) {

        sACNManager::tListener listener(weakListener);
        if (listener) {
            int universe = listener->universe();

            // Save listener
            m_universeDetails.append(universeDetails());
            m_universeDetails.last().listener = listener;

            // Universe Number Text (Parent)
            {
                m_universeDetails.last().treeUniverse =  new QTreeWidgetItem(ui->twDiag);
                m_universeDetails.last().treeUniverse->setText(0, QString(tr("Universe %1")).arg(universe));
            }

            // Merges per second (Child)
            {
                m_universeDetails.last().treeMergesPerSecond = new QTreeWidgetItem(m_universeDetails.last().treeUniverse);
                m_universeDetails.last().treeMergesPerSecond->setText(0, tr("Merges per second"));
                m_universeDetails.last().treeMergesPerSecond->setText(1, "-");
            }

            // Bind status (Child)
            {
                m_universeDetails.last().treeMergesBindStatus = new QTreeWidgetItem(m_universeDetails.last().treeUniverse);
                m_universeDetails.last().treeMergesBindStatus->setText(0, tr("Bind status"));

                m_universeDetails.last().treeMergesBindStatusUnicast = new QTreeWidgetItem(m_universeDetails.last().treeMergesBindStatus);
                m_universeDetails.last().treeMergesBindStatusUnicast->setText(0, tr("Unicast"));
                bindStatus(m_universeDetails.last().treeMergesBindStatusUnicast, listener->getBindStatus().unicast);

                m_universeDetails.last().treeMergesBindStatusMulticast = new QTreeWidgetItem(m_universeDetails.last().treeMergesBindStatus);
                m_universeDetails.last().treeMergesBindStatusMulticast->setText(0, tr("Multicast"));
                bindStatus(m_universeDetails.last().treeMergesBindStatusMulticast, listener->getBindStatus().multicast);

            }
        }
    }
    resizeDiagColumn();

    m_displayTimer = new QTimer(this);
    connect(m_displayTimer, SIGNAL(timeout()), this, SLOT(updateDisplay()));
    m_displayTimer->start(1000);

}

aboutDialog::~aboutDialog()
{
    foreach (universeDetails universeDetail, m_universeDetails) {
        delete universeDetail.treeUniverse;
    }

    m_displayTimer->deleteLater();
    delete ui;
}

void aboutDialog::updateDisplay()
{
    for (universeDetails universeDetail : m_universeDetails) {
        // Update merges per second
        auto merges = universeDetail.listener->mergesPerSecond();
        if (merges > 0)
            universeDetail.treeMergesPerSecond->setText(1, QString("%1").arg(merges));
    }
}

void aboutDialog::bindStatus(QTreeWidgetItem *treeItem, sACNRxSocket::eBindStatus bindStatus)
{
    QString bindString;
    switch (bindStatus) {
    case sACNRxSocket::BIND_UNKNOWN:
        bindString = QString(tr("Unknown"));
        break;
    case sACNRxSocket::BIND_OK:
        bindString = QString(tr("OK"));
        break;
    case sACNRxSocket::BIND_FAILED:
        bindString = QString(tr("Failed"));
        break;
    }
    treeItem->setText(1, bindString);
}

void aboutDialog::resizeDiagColumn()
{
    // Resize coloums
    for (int n = 0; n< ui->twDiag->columnCount(); n++) {
        ui->twDiag->resizeColumnToContents(n);
    }
}

void aboutDialog::on_twDiag_expanded(const QModelIndex &index)
{
    Q_UNUSED(index);

    resizeDiagColumn();
}
void aboutDialog::on_twDiag_collapsed(const QModelIndex &index)
{
    Q_UNUSED(index);

    resizeDiagColumn();
}


void aboutDialog::on_aboutDialog_finished(int result)
{
    Q_UNUSED(result);

    this->deleteLater();
}
