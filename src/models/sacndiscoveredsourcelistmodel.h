#ifndef sACNDiscoveredSourceListModel_H
#define sACNDiscoveredSourceListModel_H

#include <QObject>
#include <QAbstractTableModel>
#include <QReadWriteLock>
#include <QList>
#include <QSortFilterProxyModel>

#include "sacndiscovery.h"


class sACNSourceInfo : public QObject
{
    Q_OBJECT
public:
    sACNSourceInfo(QString sourceName, QObject *parent = Q_NULLPTR);

    QString name;
    QList<quint16> universes;
};

/**
 * @brief The sACNDiscoveredSourceListModel class provides a
 * QAbstractItemModel which represents all the "discovered" sources and universe
 * with each source as a node with universes as its children.
 */
class sACNDiscoveredSourceListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    sACNDiscoveredSourceListModel(QObject *parent = Q_NULLPTR);
     ~sACNDiscoveredSourceListModel() override;

    int indexToUniverse(const QModelIndex &index);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;

public slots:

private slots:
    void newSource(CID cid);
    void expiredSource(CID cid);
    void newUniverse(CID cid, quint16 universe);
    void expiredUniverse(CID cid, quint16 universe);

private:
    mutable QReadWriteLock rwlock_ModelIndex;
    sACNDiscoveryRX *m_discoveryInstance;
    bool m_displayDDOnlySource;

    QHash<CID, sACNSourceInfo*> m_sources;
};

/**
 * @brief The sACNDiscoveredSourceListProxy class provides a
 * model which sorts and filters the raw sACNDiscoveredSourceListModel.
 * Currently it just sorts items with parents by universe number; in future
 * it could be extended to add additional sorting and searching options
 */
class sACNDiscoveredSourceListProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    sACNDiscoveredSourceListProxy(QObject *parent = Q_NULLPTR);
protected:
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

#endif // sACNDiscoveredSourceListModel_H
