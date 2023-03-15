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

#define QT_SHAREDPOINTER_TRACK_POINTERS 1

#include "mdimainwindow.h"
#include "nicselectdialog.h"
#include "preferences.h"
#include "consts.h"
#include <QApplication>
#include <QNetworkInterface>
#include <QProcess>
#include <QMessageBox>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStandardPaths>
#include "themes.h"
#include "sacnsender.h"
#include "newversiondialog.h"
#include "firewallcheck.h"
#include "ipc.h"
#include "translationdialog.h"
#include "crash_handler.h"
#include "crash_test.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(VER_PRODUCTVERSION_STR);
    a.setOrganizationName("sACNView");
    a.setOrganizationDomain("tomsteer.net");

    // Breakpad Crash Handler
    Breakpad::CrashHandler::instance()->Init(QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    // Breakpad Crash Tester
    if (qApp->arguments().contains("CRASHTEST", Qt::CaseInsensitive)) {
        CrashTest *crashwindow = new CrashTest;
        crashwindow->show();
    }

    // Setup theme
    Themes::apply(Preferences::getInstance()->GetTheme());

    // Setup Language
    {
        TranslationDialog td(Preferences::getInstance()->GetLocale());
        if (!td.LoadTranslation())
        {
            td.exec();
            td.LoadTranslation(Preferences::getInstance()->GetLocale());
        }
    }

    // Check web (if avaliable) for new version
    VersionCheck version;

    // Setup interface
    bool newInterface = false;
    // Interface not avaliable, or last selection was offline/localhost
    if (!Preferences::getInstance()->defaultInterfaceAvailable()
            || Preferences::getInstance()->getInstance()->networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack)
        )
    {
        NICSelectDialog d;
        int result = d.exec();

        switch (result) {
            case QDialog::Accepted:
            {
                Preferences::getInstance()->setNetworkInterface(d.getSelectedInterface());
                break;
            }
            case QDialog::Rejected:
            {
                // Exit application
                return -1;
            }
        }

        newInterface = true;
    }

    // Changed to heap rather than stack,
    // so that we can destroy before cleaning up the singletons
    MDIMainWindow *w = new MDIMainWindow();
    w->restoreMdiWindows();

    // Setup IPC
    IPC ipc(w);
    if (!ipc.isListening())
    {
        // Single instance only
        delete w;
        return -1;
    }

    // Show window
    if(Preferences::getInstance()->GetSaveWindowLayout())
        w->show();
    else
        w->showMaximized();

    // Show interface name on statusbar
    if (Preferences::getInstance()->networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack))
        w->statusBar()->showMessage(QObject::tr("WORKING OFFLINE"));
    else
    {
        const QNetworkInterface iface = Preferences::getInstance()->networkInterface();

        w->statusBar()->showMessage(QObject::tr("Selected interface: %1 (%2)")
                                        .arg(iface.humanReadableName())
                                        .arg(Preferences::GetIPv4AddressString(iface)));
    }

    // Check firewall if not newly selected
    if (!newInterface) {
        foreach (QNetworkAddressEntry ifaceAddr, Preferences::getInstance()->networkInterface().addressEntries())
        {
            if (ifaceAddr.ip().protocol() == QAbstractSocket::IPv4Protocol)
            {
                FwCheck::FwCheck_t fw = FwCheck::isFWBlocked(ifaceAddr.ip());
                QMessageBox msgBox;
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setStandardButtons(QMessageBox::Ok);
                if (fw.allowed == false) {
                    msgBox.setText(QObject::tr("Incoming connections to this application are blocked by the firewall"));
                    msgBox.exec();
                } else {
                    if (fw.restricted == true) {
                        msgBox.setText(QObject::tr("Incoming connections to this application are restricted by the firewall"));
                        msgBox.exec();
                   }
                }
            }
        }
    }

    int result = a.exec();

    w->saveMdiWindows();
    delete w;

    Preferences::getInstance()->savePreferences();

    CStreamServer::shutdown();

    if(Preferences::getInstance()->RESTART_APP)
        QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
    return result;
}
