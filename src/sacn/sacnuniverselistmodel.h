#ifndef SACNUNIVERSELISTMODEL_H
#define SACNUNIVERSELISTMODEL_H

#include <QObject>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QAbstractItemModel>
#include <QHostAddress>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QReadWriteLock>
#include <list>
#include "deftypes.h"
#include "CID.h"
#include "sacnlistener.h"

#define NUM_UNIVERSES_LISTED 20

class sACNUniverseInfo;
class sACNRxSocket;

class sACNBasicSourceInfo
{
public:
    sACNBasicSourceInfo(sACNUniverseInfo *p);
    QHostAddress address;
    QString name;
    CID cid;
    sACNUniverseInfo *parent;
    QElapsedTimer timeout;
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
 * QAbstractItemModel which represents 20 universes
 * with each universe as a node with sources as its children.
 *
 * It does minimal inspection of the source packets - just enough to get name, IP and universe
 */
class sACNUniverseListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    sACNUniverseListModel(QObject *parent = NULL);
    void setStartUniverse(int start);
    int indexToUniverse(const QModelIndex &index);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
protected:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;

public slots:
    void sourceOnline(sACNSource *source);
    void sourceChanged(sACNSource *source);
    void sourceOffline(sACNSource *source);

private:
    mutable QReadWriteLock rwlock_ModelIndex;
    QMutex mutex_readPendingDatagrams;
    QList<sACNUniverseInfo *>m_universes;
    int m_start;
    QTimer *m_checkTimeoutTimer;
    QList<QSharedPointer<sACNListener>> m_listeners;
    bool m_displayDDOnlySource;
};

#endif // SACNUNIVERSELISTMODEL_H
