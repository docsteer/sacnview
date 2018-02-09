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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "consts.h"
#include <pcap.h>

#include <QTimer>
#include <QDesktopServices>
#include <QUrl>

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
    ui->DisplayName->setText(AUTHOR);

    ui->lblLicense->setText(
                tr("This application is provided under the <a href=\"http://www.apache.org/licenses/LICENSE-2.0\">Apache License, version 2.0</a>")
    );
    ui->lblQtInfo->setText(tr("This application uses the Qt Library, version %1, licensed under the <a href=\"http://www.gnu.org/licenses/lgpl.html\">GNU LGPL</a>")
                           .arg(qVersion()));
    const char *libpcap = pcap_lib_version();
    ui->lblLibs->setText(libpcap);

    connect(ui->lblLicense, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
    connect(ui->lblQtInfo, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));

    // Setup diagnostics tree
    ui->twDiag->setColumnCount(2);
    ui->twDiag->header()->hide();

    // Get listener list
    const QHash<int, QWeakPointer<sACNListener>> listenerList =
            sACNManager::getInstance()->getListenerList();

    // Sort
    QMap<int, QWeakPointer<sACNListener>> listenerListSorted;
    QHashIterator<int, QWeakPointer<sACNListener>> hi(listenerList);
    while (hi.hasNext()) {
        hi.next();
        listenerListSorted.insert(hi.key(), hi.value());
    }

    // Display Sorted!
    QMapIterator<int, QWeakPointer<sACNListener>> mi(listenerListSorted);
    while (mi.hasNext()) {
        mi.next();

        QSharedPointer<sACNListener> listener = mi.value().toStrongRef();
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
        }
    }

    m_displayTimer = new QTimer(this);
    connect(m_displayTimer, SIGNAL(timeout()), this, SLOT(updateDisplay()));
    m_displayTimer->start(1000);

}

aboutDialog::~aboutDialog()
{
    foreach (universeDetails universeDetail, m_universeDetails) {
        delete universeDetail.treeMergesPerSecond;
        delete universeDetail.treeUniverse;
    }

    m_displayTimer->deleteLater();
    delete ui;
}

void aboutDialog::updateDisplay()
{
    foreach (universeDetails universeDetail, m_universeDetails) {
        // Update merges per second
        auto merges = universeDetail.listener->mergesPerSecond();
        if (merges > 0)
            universeDetail.treeMergesPerSecond->setText(1, QString("%1").arg(merges));
    }
}

void aboutDialog::openLink(QString link)
{
    QDesktopServices::openUrl(QUrl(link));
}

void aboutDialog::on_twDiag_expanded(const QModelIndex &index)
{
    Q_UNUSED(index);

    // Resize coloums
    for (int n = 0; n< ui->twDiag->columnCount(); n++) {
        ui->twDiag->resizeColumnToContents(n);
    }
}
void aboutDialog::on_twDiag_collapsed(const QModelIndex &index)
{
    Q_UNUSED(index);

    // Resize coloums
    for (int n = 0; n< ui->twDiag->columnCount(); n++) {
        ui->twDiag->resizeColumnToContents(n);
    }
}
