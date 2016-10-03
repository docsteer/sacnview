// Copyright (c) 2015 Tom Barthel-Steer, http://www.tomsteer.net
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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

    ui->DisplayVer->setText(VERSION);
    ui->displayDate->setText(PUBLISHED_DATE);
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
    delete ui;
}

void aboutDialog::updateDisplay()
{
    ui->teDiag->clear();

    const QHash<int, sACNListener*> listenerList =
            sACNManager::getInstance()->getListenerList();

    QString data;
    data.append(QString("Reciever Threads : %1\n").arg(listenerList.count()));

    QHashIterator<int, sACNListener*> i(listenerList);
    while (i.hasNext()) {
        i.next();
        data.append(QString("Universe %1\tMerges per second %2\n")
                    .arg(i.key())
                    .arg(i.value()->mergesPerSecond()));
    }

    ui->teDiag->setPlainText(data);
}

void aboutDialog::openLink(QString link)
{
    QDesktopServices::openUrl(QUrl(link));
}
