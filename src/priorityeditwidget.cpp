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

#include "priorityeditwidget.h"
#include <QWheelEvent>
#include "consts.h"

PriorityEditWidget::PriorityEditWidget(QWidget *parent) : GridWidget(parent)
{
    for(int i=0; i<512; i++)
    {
        setCellValue(i, QString::number(DEFAULT_SACN_PRIORITY));
    }
}


void PriorityEditWidget::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;

    if(selectedCell()<0) return;

    int value = cellValue(selectedCell()).toInt();
    value += numSteps;

    if(value<MIN_SACN_PRIORITY || value>MAX_SACN_PRIORITY) return;

    setCellValue(selectedCell(), QString::number(value));
    update();

    event->accept();
}
