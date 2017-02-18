#ifndef SACNUNIVERSELISTMODEL_H
#define SACNUNIVERSELISTMODEL_H

#include <QObject>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QAbstractItemModel>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QElapsedTimer>
#include "deftypes.h"
#include "CID.h"

#define NUM_UNIVERSES_LISTED 20

class sACNUniverseInfo;

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
    Q_OBJECT;

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

protected slots:
    void readPendingDatagrams();
    void checkTimeouts();
private:
    QList<sACNUniverseInfo *>m_universes;
    int m_start;
    QTimer *m_checkTimeoutTimer;
    QUdpSocket *m_socket;
};

#endif // SACNUNIVERSELISTMODEL_H
