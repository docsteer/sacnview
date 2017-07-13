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
#include "streamingacn.h"
#include "sacnlistener.h"

#include <QTimer>
#include <QDesktopServices>

aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutDialog)
{
    ui->setupUi(this);

    if (VERSION == GIT_CURRENT_SHA1) {
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

    connect(ui->lblLicense, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
    connect(ui->lblQtInfo, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));

    m_displayTimer = new QTimer(this);
    connect(m_displayTimer, SIGNAL(timeout()), this, SLOT(updateDisplay()));
    m_displayTimer->start(1000);

}

aboutDialog::~aboutDialog()
{
    m_displayTimer->deleteLater();
    delete ui;
}

void aboutDialog::updateDisplay()
{
    ui->teDiag->clear();

    const QHash<int, QWeakPointer<sACNListener> > listenerList =
            sACNManager::getInstance()->getListenerList();

    QString data;
    data.append(QString("Reciever Threads : %1\n").arg(listenerList.count()));

    QHashIterator<int, QWeakPointer<sACNListener> > i(listenerList);
    while (i.hasNext()) {
        i.next();
        QSharedPointer<sACNListener> listener = i.value().toStrongRef();
        if (listener) {
            data.append(QString("Universe %1\tMerges per second %2\n")
                        .arg(i.key())
                        .arg(listener->mergesPerSecond()));
        }
    }

    ui->teDiag->setPlainText(data);
}

void aboutDialog::openLink(QString link)
{
    QDesktopServices::openUrl(QUrl(link));
}
