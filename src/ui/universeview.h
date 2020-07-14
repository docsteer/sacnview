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
    void refreshTitle();
    void on_btnGo_clicked();
    void on_btnPause_clicked();
    void sourceOnline(sACNSource *source);
    void sourceOffline(sACNSource *source);
    void sourceChanged(sACNSource *source);
    void levelsChanged();
    void selectedAddressChanged(int address);
    void selectedAddressesChanged(QList<int> addresses);
    void openBigDisplay(quint16 address);
    void on_btnStartFlickerFinder_clicked();
    void on_btnLogWindow_clicked();
    void listenerStarted(int universe);

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void showEvent(QShowEvent *event);
private:
    // The column order in the source table
    enum m_SC_ROWS
    {
    COL_NAME,
    COL_ONLINE,
    COL_CID,
    COL_PRIO,
    COL_SYNC,
    COL_PREVIEW,
    COL_IP,
    COL_FPS,
    COL_SEQ_ERR,
    COL_JUMPS,
    COL_VER,
    COL_DD,
    COL_SLOTS,
    COL_END
    };

    void resizeColumns();
    bool m_bindWarningShown = false;
    void checkBind();

    Ui::UniverseView *ui = nullptr;
    QHash<sACNSource *, int> m_sourceToTableRow;
    int m_selectedAddress = -1;
    sACNManager::tListener m_listener;
    QWidget *m_parentWindow = nullptr;
    bool m_displayDDOnlySource = true;
};

#endif // UNIVERSEVIEW_H
