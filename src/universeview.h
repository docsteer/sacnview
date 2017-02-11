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

#include "mergeduniverselogger.h"

class sACNListener;

namespace Ui {
class UniverseView;
}

class sACNSource;

class UniverseView : public QWidget
{
    Q_OBJECT

public:
    explicit UniverseView(QWidget *parent = 0);
    ~UniverseView();

protected slots:
    void on_btnGo_pressed();
    void on_btnPause_pressed();
    void on_btnLogToFile_pressed();
    void sourceOnline(sACNSource *source);
    void sourceOffline(sACNSource *source);
    void sourceChanged(sACNSource *source);
    void levelsChanged();
    void selectedAddressChanged(int address);
    void on_btnStartFlickerFinder_pressed();
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
    COL_IP,
    COL_FPS,
    COL_SEQ_ERR,
    COL_JUMPS,
    COL_ONLINE,
    COL_VER,
    COL_DD,
    COL_END
    };

    enum LOG_STATE
    {
        NOT_LOGGING = 0,
        LOGGING
    };

    void setUiForLoggingState(LOG_STATE state);

    Ui::UniverseView *ui;
    QHash<sACNSource *, int> m_sourceToTableRow;
    int m_selectedAddress;
    QSharedPointer<sACNListener> m_listener;
    MergedUniverseLogger *m_logger;
};

#endif // UNIVERSEVIEW_H
