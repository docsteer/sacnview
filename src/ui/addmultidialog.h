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

#ifndef ADDMULTIDIALOG_H
#define ADDMULTIDIALOG_H

#include <QDialog>
#include "sacneffectengine.h"

namespace Ui {
class AddMultiDialog;
}

class AddMultiDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddMultiDialog(QWidget *parent = 0);
    ~AddMultiDialog();

    int startUniverse();
    int universeCount();
    sACNEffectEngine::FxMode mode();
    int startAddress();
    int endAddress();
    bool startNow();
    int level();
    qreal rate();
    int priority();

private slots:
    void rangeChanged(int);
    void on_cbEffect_currentIndexChanged(int index);
    void on_slLevel_sliderMoved(int value);
    void on_slLevel_valueChanged(int value);
    void on_dlFadeRate_valueChanged(int value);
private:
    Ui::AddMultiDialog *ui;
    void setupFxControl();

};

#endif // ADDMULTIDIALOG_H
