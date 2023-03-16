#include "crash_handler.h"
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QCoreApplication>
#include <QString>
#include <QDebug>
#ifdef Q_OS_WIN
    #include "Windows.h"
#endif
 
#if defined(Q_OS_MAC)
#include "client/mac/handler/exception_handler.h"
#elif defined(Q_OS_LINUX)
#include "client/linux/handler/exception_handler.h"
#elif defined(Q_OS_WIN32)
#include "client/windows/handler/exception_handler.h"
#endif
 
namespace Breakpad {
    /************************************************************************/
    /* CrashHandlerPrivate                                                  */
    /************************************************************************/
    class CrashHandlerPrivate
    {
    public:
        CrashHandlerPrivate()
        {
            pHandler = NULL;
        }
 
        ~CrashHandlerPrivate()
        {
            delete pHandler;
        }
 
        void InitCrashHandler(const QString& dumpPath);
        static google_breakpad::ExceptionHandler* pHandler;
        static bool bReportCrashesToSystem;
    };
 
    google_breakpad::ExceptionHandler* CrashHandlerPrivate::pHandler = NULL;
    bool CrashHandlerPrivate::bReportCrashesToSystem = false;
 
    /************************************************************************/
    /* DumpCallback                                                         */
    /************************************************************************/
#if defined(Q_OS_WIN32)
    bool DumpCallback(const wchar_t* _dump_dir,const wchar_t* _minidump_id,void* context,EXCEPTION_POINTERS* exinfo,MDRawAssertionInfo* assertion,bool success)
#elif defined(Q_OS_LINUX)
    bool DumpCallback(const google_breakpad::MinidumpDescriptor &md,void *context, bool success)
#elif defined(Q_OS_MACOS)
    bool DumpCallback(const char *dump_dir, const char *minidump_id, void *context, bool succeeded)
#endif
    {
        Q_UNUSED(context);
#if defined(Q_OS_WIN32)
        Q_UNUSED(_dump_dir);
        Q_UNUSED(_minidump_id);
        Q_UNUSED(assertion);
        Q_UNUSED(exinfo);
#endif

#ifdef Q_OS_WIN
        // Show a dialog - use WinAPI
        wchar_t message[512];
        swprintf_s(message, 512, L"Unfortunately sACNView encountered an error and needs to close.\r\nPlease sumbit a crash report with the dump file located at %ls/%ls.dmp",
                   _dump_dir,
                   _minidump_id);
        MessageBox(
               NULL,
               message,
               (LPCWSTR)L"sACNView Fatal Error",
               MB_ICONERROR | MB_OK | MB_DEFBUTTON1
           );

#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
        // Output to stderr
        fprintf(stderr,
                "%s\r\n%s %s\r\n",
                "Unfortunately sACNView encountered an error and needs to close.",
                "Please sumbit a crash report with the dump file located at",
                md.path());
#endif
 
        /*
        NO STACK USE, NO HEAP USE THERE !!!
        Creating QString's, using qDebug, etc. - everything is crash-unfriendly.
        */
        return CrashHandlerPrivate::bReportCrashesToSystem ? success : true;
    }
 
    void CrashHandlerPrivate::InitCrashHandler(const QString& dumpPath)
    {
        if ( pHandler != NULL )
            return;
 
#if defined(Q_OS_WIN32)
        pHandler = new google_breakpad::ExceptionHandler(
            dumpPath.toStdWString(),
            0, // FilterCallback
            DumpCallback,
            0, // callback_context
            google_breakpad::ExceptionHandler::HANDLER_ALL);
#elif defined(Q_OS_LINUX)
        std::string pathAsStr = dumpPath.toStdString();
        google_breakpad::MinidumpDescriptor md(pathAsStr);
        pHandler = new google_breakpad::ExceptionHandler(
            md,
            /**/ 0, //
            DumpCallback,
            0, // callback_context
            true, // install_handler
            -1); // server_fd If it is -1, in-process generation will always be used.
#elif defined(Q_OS_MACOS)
        pHandler = new google_breakpad::ExceptionHandler(
            dumpPath.toStdString(),
            0, // FilterCallback
            DumpCallback,
            0, // callback_context
            true,  // install_handler
            NULL); // If port_name is NULL, in-process dump generation will be used
#endif
    }
 
    /************************************************************************/
    /* CrashHandler                                                         */
    /************************************************************************/
    CrashHandler* CrashHandler::instance()
    {
        static CrashHandler globalHandler;
        return &globalHandler;
    }
 
    CrashHandler::CrashHandler()
    {
        d = new CrashHandlerPrivate();
    }
 
    CrashHandler::~CrashHandler()
    {
        delete d;
    }
 
    void CrashHandler::setReportCrashesToSystem(bool report)
    {
        d->bReportCrashesToSystem = report;
    }
 
    bool CrashHandler::writeMinidump()
    {
        bool res = d->pHandler->WriteMinidump();
        if (res) {
            qDebug("BreakpadQt: writeMinidump() success.");
        } else {
            qWarning("BreakpadQt: writeMinidump() failed.");
        }
        return res;
    }
 
    void CrashHandler::Init( const QString& reportPath )
    {
        d->InitCrashHandler(reportPath);
    }
}
