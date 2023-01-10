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
    void startListening(int universe);
protected slots:
    void on_btnGo_pressed();
    void on_btnPause_pressed();
    void sourceOnline(sACNSource *source);
    void sourceOffline(sACNSource *source);
    void sourceChanged(sACNSource *source);
    void levelsChanged();
    void selectedAddressChanged(int address);
    void selectedAddressesChanged(QList<int> addresses);
    void openBigDisplay(quint16 address);
    void on_btnStartFlickerFinder_pressed();
    void on_btnLogWindow_pressed();
    void listenerStarted(int universe);
protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void showEvent(QShowEvent *event);
private:
    // The column order in the source table
    enum m_SC_ROWS
    {
    COL_NAME,
    COL_CID,
    COL_PRIO,
    COL_SYNC,
    COL_PREVIEW,
    COL_IP,
    COL_FPS,
    COL_SEQ_ERR,
    COL_JUMPS,
    COL_ONLINE,
    COL_VER,
    COL_DD,
    COL_SLOTS,
    COL_END
    };

    void resizeColumns();
    bool m_bindWarningShown = false;
    void checkBind();

    Ui::UniverseView *ui;
    QHash<sACNSource *, int> m_sourceToTableRow;
    static const int NO_SELECTED_ADDRESS = -1;
    int m_selectedAddress = NO_SELECTED_ADDRESS;
    sACNManager::tListener m_listener;
    QWidget *m_parentWindow;
    bool m_displayDDOnlySource;
};

#endif // UNIVERSEVIEW_H
