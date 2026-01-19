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

#ifndef NICSELECTDIALOG_H
#define NICSELECTDIALOG_H

#include <QDialog>
#include <QHostAddress>
#include <QNetworkInterface>

namespace Ui
{
    class NICSelectDialog;
}

class NICSelectDialog : public QDialog
{
    Q_OBJECT

public:

    explicit NICSelectDialog(QWidget * parent = 0);
    ~NICSelectDialog();
    QNetworkInterface getSelectedInterface() const { return m_selectedInterface; };
protected slots:
    void on_listWidget_itemSelectionChanged();
    void on_btnSelect_pressed();
    void on_btnWorkOffline_pressed();
    void on_listWidget_doubleClicked();

private:

    Ui::NICSelectDialog * ui;
    QNetworkInterface m_selectedInterface;
    QList<QNetworkInterface> m_interfaceList;
};

#endif // NICSELECTDIALOG_H
