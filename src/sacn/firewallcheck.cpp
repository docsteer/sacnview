#include <QApplication>
#include <QFileInfo>
#include <QHostAddress>
#include <QDir>
#include "firewallcheck.h"
#include "streamcommon.h"

#ifdef Q_OS_WIN
#include "netfw.h"
#include "Objbase.h"
#endif

FwCheck::FwCheck_t FwCheck::isFWBlocked(QHostAddress ip)
{
    FwCheck_t ret;

#ifdef Q_OS_WIN
    qDebug() << "Firewall Starting Check";
    HRESULT hr = S_OK;
    INetFwMgr* fwMgr = NULL;

    VARIANT allowed;
    VARIANT restricted;
    QString filename = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    BSTR bstrFilename = SysAllocString(reinterpret_cast<const OLECHAR*>(filename.utf16()));
    QString ipaddress = ip.toString();
    BSTR bstrIpaddress = SysAllocString(reinterpret_cast<const OLECHAR*>(ipaddress.utf16()));

    // Create instance of firewall manager
    hr = CoCreateInstance(
            __uuidof(NetFwMgr),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(INetFwMgr),
            (void**)&fwMgr
            );
    if (!FAILED(hr))
    {
        // Check if port is blocked
        hr = fwMgr->IsPortAllowed(
                    bstrFilename,
                    NET_FW_IP_VERSION_V4,
                    STREAM_IP_PORT,
                    bstrIpaddress,
                    NET_FW_IP_PROTOCOL_UDP,
                    &allowed,
                    &restricted
                    );

        ret.allowed = (VARIANT_TRUE  == allowed.boolVal);
        qDebug() << "Firewall - Allowed:" << ret.allowed;
        ret.restricted = (VARIANT_TRUE  == restricted.boolVal);
        qDebug() << "Firewall - Restricted:" << ret.restricted;

        fwMgr->Release();
    } else {
        qDebug() << "Firewall Check Failed" << hr;
    }

    // Clean up (SysFreeString is null pointer safe)
    SysFreeString(bstrFilename);
    SysFreeString(bstrIpaddress);

#else //Q_WS_WIN
    Q_UNUSED(ip);
#endif

    return ret;
}
