#include "logwindow.h"
#include "ui_logwindow.h"

#include <QDateTime>
#include <QClipboard>
#include <QScrollBar>

LogWindow::LogWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogWindow)
{
    ui->setupUi(this);
}

LogWindow::~LogWindow()
{
    delete ui;
}

void LogWindow::setUniverse(int universe)
{
    setWindowTitle(tr("Log - Universe %1").arg(universe));

    m_listener = sACNManager::getInstance()->getListener(universe);
    connect(m_listener.data(), SIGNAL(levelsChanged()), this, SLOT(onLevelsChanged()));

}

void LogWindow::onLevelsChanged()
{
    QString text = QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss");
    const sACNMergedSourceList list = m_listener->mergedLevels();
    for(int i=0; i<list.count(); i++)
    {
        sACNMergedAddress a = list.at(i);
        if(a.changedSinceLastMerge)
            text.append(QString(" %1@%2")
                        .arg(i+1)
                        .arg(a.level)
                        );
    }
    ui->textEdit->appendPlainText(text);
    QScrollBar *sb = ui->textEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}



void LogWindow::on_btnCopyClipboard_pressed()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(ui->textEdit->toPlainText());
}

void LogWindow::on_btnClear_pressed()
{
    ui->textEdit->clear();
}
