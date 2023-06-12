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

#include "flickerfinderinfoform.h"
#include "ui_flickerfinderinfoform.h"
#include "universedisplay.h"
#include "preferences.h"

FlickerFinderInfoForm::FlickerFinderInfoForm(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FlickerFinderInfoForm)
{
    ui->setupUi(this);
    QPalette p = ui->wLower->palette();
    p.setColor(QPalette::Window, UniverseDisplay::flickerLowerColor());
    ui->wLower->setPalette(p);
    p.setColor(QPalette::Window, UniverseDisplay::flickerHigherColor());
    ui->wHigher->setPalette(p);
    p.setColor(QPalette::Window, UniverseDisplay::flickerChangedColor());
    ui->wChange->setPalette(p);
}

FlickerFinderInfoForm::~FlickerFinderInfoForm()
{
    if(ui->cbDontShowAgain->isChecked())
    {
        Preferences::Instance().setFlickerFinderShowInfo(false);
    }
    delete ui;
}
