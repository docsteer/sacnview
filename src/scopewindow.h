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

#ifndef SCOPEWINDOW_H
#define SCOPEWINDOW_H

#define COL_UNIVERSE    0
#define COL_ADDRESS     1
#define COL_ENABLED     2
#define COL_COLOUR      3
#define COL_TRIGGER     4
#define COL_16BIT       5


class ScopeChannel;
class QTableWidgetItem;

#include <QtGui>
#include <QWidget>
#include <QButtonGroup>

namespace Ui {
class ScopeWindow;
}

class ScopeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ScopeWindow(QWidget *parent = 0);
    ~ScopeWindow();
private slots:
    void timebaseChanged(int value);
    void on_btnStart_pressed();
    void on_btnStop_pressed();
    void on_btnAddChannel_pressed();
    void on_btnRemoveChannel_pressed();
    void on_tableWidget_cellDoubleClicked(int row, int col);
    void on_tableWidget_itemChanged(QTableWidgetItem * item);
private:
    Ui::ScopeWindow *ui;
    QList<ScopeChannel *> m_channels;
    QButtonGroup *m_radioGroup;
};

#endif // SCOPEWINDOW_H
