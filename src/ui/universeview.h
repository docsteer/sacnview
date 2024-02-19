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

#ifndef UNIVERSEVIEW_H
#define UNIVERSEVIEW_H

#include <QWidget>
#include "consts.h"
#include "streamingacn.h"

class sACNListener;
class SACNSourceTableModel;

namespace Ui {
class UniverseView;
}

class sACNSource;

class UniverseView : public QWidget
{
    Q_OBJECT

public:
    explicit UniverseView(int universe = MIN_SACN_UNIVERSE, QWidget *parent = 0);
    ~UniverseView();

    // Trigger API
    Q_SLOT void startRx() { on_btnGo_clicked(); }
    Q_SLOT void stopRx() { on_btnPause_clicked(); }

protected slots:
    void refreshTitle();
    void on_btnGo_clicked();
    void on_btnPause_clicked();
    void sourceChanged(sACNSource *source);
    void levelsChanged();
    void selectedAddressChanged(int address);
    void selectedAddressesChanged(QList<int> addresses);
    void openBigDisplay(quint16 address);
    void on_btnStartFlickerFinder_clicked();
    void on_btnCompareUniverse_clicked();
    void on_sbCompareUniverse_editingFinished();
    void on_btnClearOffline_clicked();
    void on_btnLogWindow_clicked();
    void on_btnExportSourceList_clicked();
    void listenerStarted(int universe);

    void onFlickerFinderChanged();
    void onCompareUniverseChanged();

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void showEvent(QShowEvent *event);

    void startListening(int universe);

private:
    void resizeColumns();
    bool m_bindWarningShown = false;
    void checkBind();

    void updateButtons(bool running);

    QString prioText(const sACNSource *source, quint8 address) const;

    Ui::UniverseView *ui = nullptr;
    SACNSourceTableModel* m_sourceTableModel = nullptr;
    static constexpr int NO_SELECTED_ADDRESS = -1;
    int m_selectedAddress = NO_SELECTED_ADDRESS;
    sACNManager::tListener m_listener;
    QWidget *m_parentWindow = nullptr;
    bool m_displayDDOnlySource = true;
};

#endif // UNIVERSEVIEW_H
