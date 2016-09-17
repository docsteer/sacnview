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

#include "scopewindow.h"
#include "ui_scopewindow.h"
#include <QRadioButton>

static const QList<int> COLOURS({
                                    Qt::white,
                                    Qt::red	,
                                    Qt::darkRed,
                                    Qt::green,
                                    Qt::darkGreen,
                                    Qt::blue,
                                    Qt::darkBlue,
                                    Qt::cyan,
                                    Qt::darkCyan,
                                    Qt::magenta,
                                    Qt::darkMagenta,
                                    Qt::yellow,
                                    Qt::darkYellow
                                });

static const QList<int> TIMEBASES({
    1000,
    500,
    200,
    100,
    50,
    20,
    10,
    5,
    2,
    1
});

ScopeWindow::ScopeWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScopeWindow)
{
    ui->setupUi(this);
    ui->dlTimebase->setMinimum(0);
    ui->dlTimebase->setMaximum(TIMEBASES.count() - 1);
    connect(ui->dlTimebase, SIGNAL(valueChanged(int)), this, SLOT(timebaseChanged(int)));
    ui->tableWidget->setRowCount(0);
}

ScopeWindow::~ScopeWindow()
{
    delete ui;
}

void ScopeWindow::timebaseChanged(int value)
{
    int timebase = TIMEBASES[value];
    ui->lbTimebase->setText(tr("%1 ms/div").arg(timebase));
    ui->widget->setTimebase(timebase);
}

void ScopeWindow::on_btnStart_pressed()
{
    ui->widget->start();
}

void ScopeWindow::on_btnStop_pressed()
{
    ui->widget->stop();
}

void ScopeWindow::on_btnAddChannel_pressed()
{
    int rowNumber =  m_channels.count();
    ScopeChannel *channel = new ScopeChannel(1, rowNumber % 512);
    QColor col((Qt::GlobalColor)COLOURS[rowNumber % COLOURS.count()]);
    channel->setColor(col);
    ui->widget->addChannel(channel);
    m_channels << channel;
    ui->tableWidget->setRowCount(m_channels.count());
    QTableWidgetItem *item = new QTableWidgetItem(QString::number(channel->universe()));
    ui->tableWidget->setItem(m_channels.count()-1, 0, item);
    item = new QTableWidgetItem(QString::number(channel->address()+1));
    ui->tableWidget->setItem(m_channels.count()-1, 1, item);

    item = new QTableWidgetItem(tr("Yes"));
    item->setCheckState(Qt::Checked);
    ui->tableWidget->setItem(m_channels.count()-1, 2, item);

    item = new QTableWidgetItem();
    item->setBackgroundColor(col);
    ui->tableWidget->setItem(m_channels.count()-1, 3, item);

    ui->tableWidget->setCellWidget(m_channels.count()-1, 4, new QRadioButton(this));

}

void ScopeWindow::on_btnRemoveChannel_pressed()
{

}
