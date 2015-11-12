#include "priorityeditwidget.h"
#include <QWheelEvent>
#include "consts.h"

PriorityEditWidget::PriorityEditWidget(QWidget *parent) : GridWidget(parent)
{
    for(int i=0; i<512; i++)
    {
        setCellValue(i, 100);
    }
}


void PriorityEditWidget::wheelEvent(QWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;

    if(selectedCell()<0) return;

    int value = cellValue(selectedCell());
    value += numSteps;

    if(value<MIN_SACN_PRIORITY || value>MAX_SACN_PRIORITY) return;

    setCellValue(selectedCell(), value);
    update();

    event->accept();
}
