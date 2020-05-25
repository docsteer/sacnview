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
#include "sacnsender.h"
#include "versioncheck.h"
#include "firewallcheck.h"
#include "ipc.h"
#include "theme/darkstyle.h"
#include "translations/translationdialog.h"
#ifdef USE_BREAKPAD
    #include "crash_handler.h"
    #include "crash_test.h"
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName(APP_NAME);
    a.setApplicationVersion(VERSION);
    a.setOrganizationName("sACNView");
    a.setOrganizationDomain("tomsteer.net");

#ifdef USE_BREAKPAD
    // Breakpad Crash Handler
    Breakpad::CrashHandler::instance()->Init(QStandardPaths::writableLocation(QStandardPaths::TempLocation));

    // Breakpad Crash Tester
    if (qApp->arguments().contains("CRASHTEST", Qt::CaseInsensitive)) {
        CrashTest *crashwindow = new CrashTest;
        crashwindow->show();
    }
#endif

    // Setup Language
    {
        TranslationDialog td(Preferences::getInstance()->GetLocale());
        if (!td.LoadTranslation())
        {
            td.exec();
            td.LoadTranslation(Preferences::getInstance()->GetLocale());
        }
    }

    // Windows XP Support
    #ifdef Q_OS_WIN
        #if (TARGET_WINXP)
            #pragma message("This binary is intended for Windows XP ONLY")
            QSysInfo systemInfo;
            QMessageBox msgBox;
            msgBox.setStandardButtons(QMessageBox::Ok);
            if (
                (systemInfo.kernelVersion().startsWith(QString("5.1"))) // Windows XP 32bit
                || (systemInfo.kernelVersion().startsWith(QString("5.2")))) // Windows XP 64bit
            {
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setText(QObject::tr("This binary is intended for Windows XP only\r\nThere are major issues mixed IPv4 and IPv6 enviroments\r\n\r\nPlease ensure IPv6 is disabled"));
                msgBox.exec();
            } else {
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.setText(QObject::tr("This binary is intended for Windows XP only"));
                msgBox.exec();
                a.exit();
                return -1;
            }
        #else
            #pragma message("This binary is intended for Windows >= 7")
        #endif
    #endif

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


    a.setStyle(QStyleFactory::create("Fusion"));
    if(Preferences::getInstance()->GetTheme() == Preferences::THEME_DARK)
    {
        a.setStyle(new DarkStyle);
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
