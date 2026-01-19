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

#ifndef GRIDEDITWIDGET_H
#define GRIDEDITWIDGET_H

#include "gridwidget.h"
#include <QObject>
#include <QWidget>

/**
 * @brief The GridEditWidget class provides a widget, based on the grid widget, which allows
 * editing of per channel priority values
 */
class GridEditWidget : public GridWidget
{
    Q_OBJECT
public:

    GridEditWidget(QWidget * parent = Q_NULLPTR);
    void setMinimum(int min) { m_minimum = min; }
    void setMaximum(int max) { m_maximum = max; }
    void setAllValues(int value);
signals:
    void levelsSet(QList<QPair<int, int>> setLevels);

protected:

    virtual void wheelEvent(QWheelEvent * event);

private:

    int m_minimum = 0;
    int m_maximum = 100;
};

#endif // GRIDEDITWIDGET_H
