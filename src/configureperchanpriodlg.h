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


#ifndef CONFIGUREPERCHANPRIODLG_H
#define CONFIGUREPERCHANPRIODLG_H

#include <QDialog>
#include "deftypes.h"
#include "consts.h"

namespace Ui {
class ConfigurePerChanPrioDlg;
}

class ConfigurePerChanPrioDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigurePerChanPrioDlg(QWidget *parent = 0);
    ~ConfigurePerChanPrioDlg();
    void setData(uint1 *data);
    uint1 *data();
public slots:
    void on_sbPriority_valueChanged(int value);
    void on_btnSetAll_pressed();
    void on_widget_selectedCellChanged(int cell);
private:
    Ui::ConfigurePerChanPrioDlg *ui;
    uint1 m_data[MAX_DMX_ADDRESS];
};

#endif // CONFIGUREPERCHANPRIODLG_H
