// Copyright (c) 2015 Electronic Theatre Controls, http://www.etcconnect.com
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
