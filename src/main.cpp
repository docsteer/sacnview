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
#include <QSurfaceFormat>

#include "themes.h"
#include "sacnsender.h"
#include "newversiondialog.h"
#include "firewallcheck.h"
#include "ipc.h"
#include "translations/translationdialog.h"

#ifdef USE_BREAKPAD
    #include "crash_handler.h"
    #include "crash_test.h"
#endif

int main(int argc, char *argv[])
{
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", QByteArray());
    qputenv("QT_SCALE_FACTOR", QByteArray());

    // Share the OpenGL Contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    {
      QSurfaceFormat format;
      QSurfaceFormat::setDefaultFormat(format);
    }

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

    // Setup theme
    Themes::apply(Preferences::Instance().GetTheme());

    // Setup Language
    {
        TranslationDialog td(Preferences::Instance().GetLocale());
        if (!td.LoadTranslation())
        {
            td.exec();
            td.LoadTranslation(Preferences::Instance().GetLocale());
        }
    }

    // Check web (if enabled) for new version
    VersionCheck *versionCheck = nullptr;
    if (Preferences::Instance().GetAutoCheckUpdates())
    {
      versionCheck = new VersionCheck();
      versionCheck->checkForUpdate();
    }

    // Setup interface
    bool newInterface = false;
    // Interface not avaliable, or last selection was offline/localhost
    if (!Preferences::Instance().defaultInterfaceAvailable()
            || Preferences::Instance().networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack)
        )
    {
        NICSelectDialog d;
        int result = d.exec();

        switch (result) {
            case QDialog::Accepted:
            {
                Preferences::Instance().setNetworkInterface(d.getSelectedInterface());
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

#ifdef NDEBUG
    // Setup IPC
    IPC ipc(w);
    if (!ipc.isListening())
    {
        // Single instance only
        delete w;
        return -1;
    }
#endif

    // Show window
    if (Preferences::Instance().GetWindowMode() == WindowMode::Floating)
    {
      // Restore after show to place the floating windows on top
      w->show();
      w->restoreSubWindows();
    }
    else if (Preferences::Instance().GetRestoreWindowLayout())
    {
      // MDI must restore before showing
      w->restoreSubWindows();
      w->show();
    }
    else
    {
      w->showMaximized();
    }

    // Show interface name on statusbar
    if (Preferences::Instance().networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack))
        w->statusBar()->showMessage(QObject::tr("WORKING OFFLINE"));
    else
    {
        const QNetworkInterface iface = Preferences::Instance().networkInterface();

        w->statusBar()->showMessage(QObject::tr("Selected interface: %1 (%2)")
                                        .arg(iface.humanReadableName())
                                        .arg(Preferences::GetIPv4AddressString(iface)));
    }

    // Check firewall if not newly selected
    if (!newInterface) {
        foreach (QNetworkAddressEntry ifaceAddr, Preferences::Instance().networkInterface().addressEntries())
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

    w->saveSubWindows();
    delete w;
    delete versionCheck;

    Preferences::Instance().savePreferences();

    CStreamServer::shutdown();

    if(Preferences::Instance().GetRestartPending())
        QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
    return result;
}
