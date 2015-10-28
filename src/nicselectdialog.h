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

#ifndef NICSELECTDIALOG_H
#define NICSELECTDIALOG_H

#include <QDialog>
#include <QHostAddress>
#include <QNetworkInterface>

namespace Ui {
class NICSelectDialog;
}

class NICSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NICSelectDialog(QWidget *parent = 0);
    ~NICSelectDialog();
    QNetworkInterface getSelectedInterface() const {return m_selectedInterface;};
protected slots:
    void on_listWidget_itemSelectionChanged();
    void on_btnSelect_pressed();
    void on_btnWorkOffline_pressed();
private:
    Ui::NICSelectDialog *ui;
    QNetworkInterface m_selectedInterface;
    QList<QNetworkInterface> m_interfaceList;
};

#endif // NICSELECTDIALOG_H
