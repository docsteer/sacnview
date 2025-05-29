#include "ipc.h"
#include "consts.h"
#include <QString>

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
        m_pipe->removeServer(APP_NAME);
        m_pipe->listen(APP_NAME);

        connect(m_pipe, &QLocalServer::newConnection, this, &IPC::newConnection);
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
    connect(ipcClient, &IPC_Client::foreground, this, &IPC::foreground);
}

void IPC::foreground()
{
    qDebug() << "Another instance attempted to run. Bring main window to foreground.";
    main_window->activateWindow();
    main_window->raise();
}




IPC_Client::IPC_Client(QLocalSocket *client, QObject *parent):
    QObject(parent),
    m_client(client),
    m_sender()
{
    connect(m_client, &QLocalSocket::disconnected, this, &IPC_Client::deleteLater);
    connect(m_client, &QLocalSocket::readyRead, this, &IPC_Client::readyRead);

    QString sVersion = QString("VERSION,%1 %2").arg(APP_NAME).arg(VERSION);
    m_client->write(sVersion.toUtf8());
    m_client->flush();

    qDebug() << "IPC" << qint64(m_client) << ": Connected";
}

IPC_Client::~IPC_Client()
{
    qDebug() << "IPC" << qint64(m_client) << ": Disconnected";
    if(m_sender)
        m_sender->deleteLater();
}

void IPC_Client::readyRead()
{
    QByteArray data = m_client->readAll();
    if (data.isEmpty()) return;
    QString s_data = QString::fromLatin1(data.data());
    if (s_data.isEmpty()) return;
    s_data = s_data.simplified();
    s_data = s_data.toUpper();

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
        m_listener = sACNManager::Instance().getListener(universe);
        connect(m_listener.data(), &sACNListener::levelsChanged, this, &IPC_Client::levelsChanged);
        levelsChanged();

        qDebug() << "IPC" << qint64(m_client) << ": Listening to universe" << universe;
    }

    /*
     * Send to universe
     *
     * SYNTAX: SEND,[Universe Number],[Source Name],[Array of values]
     *
     */
    if (l_data[0] == "SEND")
    {
        if (l_data.count() != 4) return;

        int universe = l_data[1].toInt();
        QString sourceName = l_data[2];
        QByteArray levels = data.mid(
                l_data[0].size() + 1 +
                l_data[1].size() + 1 +
                l_data[2].size() + 1,
                MAX_DMX_ADDRESS);

        if(!m_sender)
        {
            m_sender = new sACNSentUniverse(universe);
        }

        if (m_sender->universe() != universe)
        {
            qDebug() << "IPC" << qint64(m_client) << ": Sending to universe" << universe;
            m_sender->setUniverse(universe);
        }

        if (m_sender->name() != sourceName)
        {
            qDebug() << "IPC" << qint64(m_client) << ": Sending as" << sourceName;
            m_sender->setName(sourceName);
        }

        if (m_sender->isSending() == false)
            m_sender->startSending(true);

        for (quint16 n = 0; n < levels.length(); n++)
        {
            m_sender->setLevel(n, levels.at(n));
        }
    }
}

void IPC_Client::levelsChanged()
{
    /*
     * Send updated values to pipe
     *
     * SYNTAX: UNIVERSE,[Universe Number],[Array of values]
     *
     */

    sACNMergedSourceList m_sources;
    m_sources = m_listener->mergedLevels();

    QString s_data = QString("UNIVERSE,%1,").arg(m_listener->universe());

    for (int n = 0; n < m_sources.count(); n++)
    {
        s_data.append(QChar(std::max(m_sources[n].level, 0)));
    }

    m_client->write(s_data.toLatin1());
    m_client->flush();
}
