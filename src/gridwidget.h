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
#include <QList>

/**
 * @brief The GridWidget class provides a control for a grid of 512 numeric values, with
 * configurable color backgrounds and selection. Used in the app for showing and editing DMX style values
 */
class GridWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GridWidget(QWidget *parent = Q_NULLPTR);

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
    QList<int> selectedCells() const { return m_selectedAddresses; }
    /**
      * @brief setMultiSelect sets whether multiple cells can be selected at once
      **/
    void setMultiSelect(bool value) {m_allowMultiSelect = value;}
signals:
    // The user changed the selected address. -1 means no selected address
    void selectedCellsChanged(QList<int> selectedCells);
    // User double clicked on a cell.
    void cellDoubleClick(quint16 address);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual QSize minimumSizeHint() const override;
    virtual QSize sizeHint() const override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual bool event(QEvent *event) override;

    // Returns the cell under point, -1 for none
    int cellHitTest(const QPoint &point);

private:
    QList<int> m_selectedAddresses;
    QVector<QColor> m_colors;
    QStringList m_values;
    bool m_allowMultiSelect = false;
protected:
    int m_cellHeight;
};

#endif // GRIDWIDGET_H
