#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "consts.h"
#include "streamingacn.h"
#include "sacnlistener.h"

#include <QTimer>

aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutDialog)
{
    ui->setupUi(this);

    ui->DisplayVer->setText(VERSION);
    ui->displayDate->setText(PUBLISHED_DATE);
    ui->DisplayName->setText(AUTHOR);

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
