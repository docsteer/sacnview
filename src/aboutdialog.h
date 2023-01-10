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

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "streamingacn.h"
#include "sacnlistener.h"
#include <QDialog>
#include <QTreeWidgetItem>
#include <QTreeWidget>

namespace Ui {
class aboutDialog;
}

class aboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit aboutDialog(QWidget *parent = Q_NULLPTR);
    ~aboutDialog();

private slots:
    void updateDisplay();

    void on_twDiag_expanded(const QModelIndex &index);
    void on_twDiag_collapsed(const QModelIndex &index);

    void on_aboutDialog_finished(int result);

private:
    Ui::aboutDialog *ui;
    QTimer *m_displayTimer;

    struct universeDetails
    {
        universeDetails() {}

        sACNManager::tListener listener;
        QTreeWidgetItem* treeUniverse;
        QTreeWidgetItem* treeMergesPerSecond;
        QTreeWidgetItem* treeMergesBindStatus;
        QTreeWidgetItem* treeMergesBindStatusUnicast;
        QTreeWidgetItem* treeMergesBindStatusMulticast;
    };
    QList<universeDetails> m_universeDetails;

    void resizeDiagColumn();
    void bindStatus(QTreeWidgetItem *treeItem, sACNRxSocket::eBindStatus bindStatus);
};

#endif // ABOUTDIALOG_H
