#ifndef PRIORITYEDITWIDGET_H
#define PRIORITYEDITWIDGET_H

#include <QObject>
#include <QWidget>
#include "gridwidget.h"

/**
 * @brief The PriorityEditWidget class provides a widget, based on the grid widget, which allows
 * editing of per channel priority values
 */
class PriorityEditWidget : public GridWidget
{
public:
    PriorityEditWidget(QWidget *parent = 0);
protected:
    virtual void wheelEvent(QWheelEvent *event);
};

#endif // PRIORITYEDITWIDGET_H
