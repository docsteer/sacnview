// Copyright 2023 Richard Thompson
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

#pragma once

#include <QWidget>

#include "streamingacn.h"

#include <map>

class sACNListener;
class SACNSourceTableModel;

namespace Ui
{
    class MultiView;
}

class MultiView : public QWidget
{
    Q_OBJECT
public:

    explicit MultiView(QWidget * parent = 0);
    explicit MultiView(int firstUniverse, QWidget * parent = 0);
    ~MultiView();

    // Trigger API
    Q_SLOT void startRx() { on_btnStartStop_clicked(true); }
    Q_SLOT void stopRx() { on_btnStartStop_clicked(false); }

    Q_INVOKABLE QJsonObject getJsonConfiguration() const;
    Q_INVOKABLE void setJsonConfiguration(const QJsonObject & json);

protected slots:
    void on_btnStartStop_clicked(bool checked);
    void on_btnClearOffline_clicked();
    void on_btnResetCounters_clicked();
    void on_btnExport_clicked();

private:

    Ui::MultiView * ui = nullptr;
    SACNSourceTableModel * m_sourceTableModel = nullptr;
    std::map<uint16_t, sACNManager::tListener> m_listeners;
};
