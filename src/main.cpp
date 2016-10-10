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


#include "mdimainwindow.h"
#include "nicselectdialog.h"
#include "preferences.h"
#include "consts.h"
#include <QApplication>
#include <QNetworkInterface>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(VERSION);
    a.setOrganizationName(AUTHOR);
    a.setOrganizationDomain("tomsteer.net");

    if(!Preferences::getInstance()->defaultInterfaceAvailable())
    {
        NICSelectDialog d;
        int result = d.exec();

        if(result==QDialog::Accepted)
        {
            Preferences::getInstance()->setNetworkInterface(d.getSelectedInterface());
        }
    }


    MDIMainWindow w;
    w.showMaximized();

    int result = a.exec();

    Preferences::getInstance()->savePreferences();

    return result;
}
