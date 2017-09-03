#include "ipc.h"
#include "consts.h"
#include <QSTRING>

#define TIME_OUT (500)    // 500ms

IPC::IPC(MDIMainWindow *w, QObject* parent):
    QObject(parent),
    main_window(w)
{
    m_pipe = new QLocalServer(this);

    // Test if already open
    QLocalSocket socket;
    socket.connectToServer(APP_NAME, QIODevice::ReadWrite);
    if(socket.waitForConnected(TIME_OUT)) {
        // Already open, send bring to foreground command
        qDebug() << "Instance already open, setting that to foreground";
        socket.write(QByteArray("FOREGROUND"));
        socket.flush();
        socket.waitForBytesWritten(TIME_OUT);
    } else {
        // Create new
        m_pipe->listen(APP_NAME);

        connect(m_pipe, SIGNAL(newConnection()), this, SLOT(newConnection()));
    }
}

IPC::~IPC()
{
    m_pipe->close();
    m_pipe->deleteLater();
}

void IPC::newConnection()
{
    IPC_Client *ipcClient = new IPC_Client(m_pipe->nextPendingConnection());
    connect(ipcClient, SIGNAL(foreground()), this, SLOT(foreground()));
}

void IPC::foreground()
{
    qDebug() << "Another instance attempted to run. Bring main window to foreground.";
    main_window->activateWindow();
    main_window->raise();
}




IPC_Client::IPC_Client(QLocalSocket *client, QObject *parent):
    QObject(parent),
    m_client(client)
{
    connect(m_client, SIGNAL(disconnected()), this, SLOT(deleteLater()));
    connect(m_client, SIGNAL(readyRead()), this, SLOT(readyRead()));

    QString sVersion = QString("VERSION,%1 %2").arg(APP_NAME).arg(VERSION);
    m_client->write(sVersion.toUtf8());
    m_client->flush();

    qDebug() << "IPC" << qint64(m_client) << ": Connected";
}

IPC_Client::~IPC_Client()
{
    qDebug() << "IPC" << qint64(m_client) << ": Disconnected";
}

void IPC_Client::readyRead()
{
    QByteArray data = m_client->readAll();
    if (data.isEmpty()) return;
    QString s_data = QString::fromLatin1(data.data());
    if (s_data.isEmpty()) return;
    s_data = s_data.simplified();
    s_data = s_data.toUpper();
    qDebug() << "IPC" << qint64(m_client) << ":" << s_data;

    QStringList l_data = s_data.split(',');
    if (l_data.isEmpty()) return;

    /*
     * Another instance has attempted to run
     * They have requested that this instance comes for foreground
     *
     */
    if (l_data[0] == "FOREGROUND") {
        emit foreground();
        return;
    }

    /*
     * Listen to universe
     *
     * SYNTAX: LISTEN,[Universe Number]
     *
     */
    if (l_data[0] == "LISTEN")
    {
        if (l_data.count() != 2) return;

        int universe = l_data[1].toInt();
        if (m_listener) m_listener->disconnect(this);
        m_listener = sACNManager::getInstance()->getListener(universe);
        connect(m_listener.data(), SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));
        levelsChanged();

        qDebug() << "IPC" << qint64(m_client) << ": Listening to universe" << universe;
    }
}

void IPC_Client::levelsChanged()
{
    sACNMergedSourceList m_sources;
    m_sources = m_listener->mergedLevels();

    QString s_data = QString("UNIVERSE,%1,").arg(m_listener->universe());

    for (unsigned int n = 0; n < m_sources.count(); n++)
    {
        s_data.append(QChar(std::max(m_sources[n].level, 0)));
    }
    m_client->write(s_data.toLatin1());
    m_client->flush();
}
