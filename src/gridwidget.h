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

#ifndef GRIDWIDGET_H
#define GRIDWIDGET_H

#include <QWidget>

/**
 * @brief The GridWidget class provides a control for a grid of 512 numeric values, with
 * configurable color backgrounds and selection. Used in the app for showing and editing DMX style values
 */
class GridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GridWidget(QWidget *parent = 0);

    void setCellColor(int cell, const QColor &color);
    /**
     * @brief setCellValue - sets the value for a cell
     * @param cell - The cell number to set, 0-511
     * @param value - The string value to set
     */
    void setCellValue(int cell, const QString &value);
    /**
     * @brief cellValue returns the value of the specified cell
     * @param cell - which cell, 0-511
     * @return the value
     */
    QString cellValue(int cell);
    /**
     * @brief selectedCell returns the currently selected cell, or -1 if no cell is selected
     * @return the selected cell, or -1 if no cell selected
     */
    int selectedCell() const { return m_selectedAddress;};
signals:
    // The user changed the selected address. -1 means no selected address
    void selectedCellChanged(int cell);
protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

    // Returns the cell under point, -1 for none
    int cellHitTest(const QPoint &point);

private:
    int m_selectedAddress;
    QList<QColor> m_colors;
    QStringList m_values;
};

#endif // GRIDWIDGET_H
