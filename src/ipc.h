#ifndef IPC_H
#define IPC_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include "mdimainwindow.h"
#include "sacnlistener.h"

class IPC : public QObject
{
    Q_OBJECT
public:
    IPC(MDIMainWindow *w, QObject *parent = 0);
    virtual ~IPC();
    bool isListening() {return m_pipe->isListening();}

private:
    QLocalServer *m_pipe;
    MDIMainWindow *main_window;

private slots:
    void newConnection();
    void foreground();
};

class IPC_Client : public QObject
{
    Q_OBJECT
public:
    IPC_Client(QLocalSocket *client, QObject *parent = 0);
    virtual ~IPC_Client();

private:
    QLocalSocket *m_client;
    QSharedPointer<sACNListener>m_listener;

private slots:
    void readyRead();
    void levelsChanged();

signals:
    void foreground();
};

#endif // IPC_H
