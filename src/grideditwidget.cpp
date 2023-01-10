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

#include "grideditwidget.h"
#include <QWheelEvent>
#include "consts.h"

GridEditWidget::GridEditWidget(QWidget *parent) : GridWidget(parent)
{
    setMultiSelect(true);
}


void GridEditWidget::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;

    QList<QPair<int, int>> level_list;

    QListIterator<int>i(selectedCells());
    while(i.hasNext())
    {
        int cell = i.next();
        int value = cellValue(cell).toInt();
        value += numSteps;

        if(!(value<m_minimum || value>m_maximum))
        {
            setCellValue(cell, QString::number(value));
            level_list << QPair<int,int>(cell, value);
        }
    }

    if(level_list.count()>0)
        emit levelsSet(level_list);

    update();

    event->accept();
}

void GridEditWidget::setAllValues(int value)
{
    for(int i=0; i<512; i++)
    {
        setCellValue(i, QString::number(value));
    }
}
