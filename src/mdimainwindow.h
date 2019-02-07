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

#ifndef MDIMAINWINDOW_H
#define MDIMAINWINDOW_H

#include <QMainWindow>

class sACNUniverseListModel;
class sACNDiscoveredSourceListModel;
class sACNSourceListProxy;

namespace Ui {
class MDIMainWindow;
}

class MDIMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MDIMainWindow(QWidget *parent = 0);
    ~MDIMainWindow();

    void showWidgetAsMdiWindow(QWidget *w);

    void saveMdiWindows();
    void restoreMdiWindows();
protected slots:
    void on_actionScopeView_triggered(bool checked);
    void on_actionRecieve_triggered(bool checked);
    void on_actionTranmsit_triggered(bool checked);
    void on_actionSettings_triggered(bool checked);
    void on_actionSnapshot_triggered(bool checked);
    void on_btnUnivListBack_pressed();
    void on_btnUnivListForward_pressed();
    void on_sbUniverseList_valueChanged(int value);
    void universeDoubleClick(const QModelIndex &index);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
private slots:
    void on_actionAbout_triggered(bool checked);

    void on_actionMultiUniverse_triggered();

    void on_actionPCAPPlayback_triggered();

    void on_pbFewer_clicked();

    void on_pbMore_clicked();
private:
    Ui::MDIMainWindow *ui;
    sACNUniverseListModel *m_model;
    sACNDiscoveredSourceListModel *m_modelDiscovered;
    sACNSourceListProxy *m_proxy;
    int getSelectedUniverse();
};

#endif // MDIMAINWINDOW_H
