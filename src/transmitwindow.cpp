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

#include "transmitwindow.h"
#include "ui_transmitwindow.h"
#include "consts.h"
#include "sacn/ACNShare/deftypes.h"
#include "sacn/ACNShare/ipaddr.h"
#include "sacn/streamcommon.h"
#include <QToolButton>

transmitwindow::transmitwindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::transmitwindow)
{
    ui->setupUi(this);

    ui->sbUniverse->setMinimum(1);
    ui->sbUniverse->setMaximum(MAX_SACN_UNIVERSE);

    ui->sbPriority->setMinimum(MIN_SACN_PRIORITY);
    ui->sbPriority->setMaximum(MAX_SACN_PRIORITY);

    ui->leSourceName->setText(DEFAULT_SOURCE_NAME);

    // Create preset buttons
    QHBoxLayout *layout = new QHBoxLayout();
    for(int i=0; i<PRESET_COUNT; i++)
    {
        QToolButton *button = new QToolButton(this);
        button->setText(QString::number(i+1));
        layout->addWidget(button);
    }
    ui->gbPresets->setLayout(layout);

    // Create faders
    QVBoxLayout *mainLayout = new QVBoxLayout();
    for(int row=0; row<2; row++)
    {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        for(int col=0; col<24; col++)
        {
            QVBoxLayout *faderVb = new QVBoxLayout();
            QSlider *slider = new QSlider(this);
            m_sliders << slider;
            slider->setMinimum(MIN_SACN_LEVEL);
            slider->setMaximum(MAX_SACN_LEVEL);
            connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(on_sliderMoved(int)));
            QLabel *label = new QLabel(this);
            m_sliderLabels << label;
            label->setText(QString("%1\r\n%2")
                           .arg(row*24 + col + 1)
                           .arg(0)
                           );
            faderVb->addWidget(slider);
            faderVb->addWidget(label);
            rowLayout->addLayout(faderVb);
        }
        mainLayout->addLayout(rowLayout);
    }
    ui->gbFaders->setLayout(mainLayout);
}

transmitwindow::~transmitwindow()
{
    delete ui;
}

void transmitwindow::on_sbUniverse_valueChanged(int value)
{
    CIPAddr address;
    GetUniverseAddress(value, address);

    char string[CIPAddr::ADDRSTRINGBYTES];
    CIPAddr::AddrIntoString(address, string, false, false);

    ui->rbMulticast->setText(tr("Multicast to %1").arg(string));
}

void transmitwindow::on_sliderMoved(int value)
{
    int index = m_sliders.indexOf(qobject_cast<QSlider*>(sender()));
    if(index<0) return;

    QLabel *label = m_sliderLabels[index];

    label->setText(QString("%1\r\n%2")
    .arg(index+1)
    .arg(value)
    );

}
