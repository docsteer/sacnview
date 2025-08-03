#ifndef SACNUNIVERSELISTMODEL_H
#define SACNUNIVERSELISTMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QHostAddress>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QReadWriteLock>
#include <list>
#include "CID.h"
#include "streamingacn.h"

class sACNUniverseInfo;
class sACNRxSocket;

class sACNBasicSourceInfo
{
public:
    sACNBasicSourceInfo(sACNUniverseInfo *p);
    QHostAddress address;
    QString name;
    CID cid;
    QElapsedTimer timeout;
    int universe;
};

class sACNUniverseInfo
{
public:
    sACNUniverseInfo(int u);
    ~sACNUniverseInfo();
    int universe;
    QList<sACNBasicSourceInfo *>sources;
    QHash<CID, sACNBasicSourceInfo *>sourcesByCid;
};

/**
 * @brief The sACNUniverseListModel class provides a
 * QAbstractItemModel which represents x universes
 * with each universe as a node with sources as its children.
 *
 * It does minimal inspection of the source packets - just enough to get name, IP and universe
 */
class sACNUniverseListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    sACNUniverseListModel(QObject *parent = NULL);
    Q_SLOT void setStartUniverse(int start);
    int indexToUniverse(const QModelIndex &index) const;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
protected:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;

public slots:
    void listenerStarted(int universe);
    void sourceOnline(sACNSource *source);
    void sourceChanged(sACNSource *source);
    void sourceOffline(sACNSource *source);

private:
    mutable QReadWriteLock rwlock_ModelIndex;
    QMutex mutex_readPendingDatagrams;
    QList<sACNUniverseInfo *>m_universes;
    QList<bool> m_universeBindOk;
    int m_start;
    QTimer *m_checkTimeoutTimer;
    QList<sACNManager::tListener> m_listeners;
    bool m_displayDDOnlySource;
};

#endif // SACNUNIVERSELISTMODEL_H
