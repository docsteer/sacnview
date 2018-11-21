#ifndef sACNDiscoveredSourceListModel_H
#define sACNDiscoveredSourceListModel_H

#include <QObject>
#include <QAbstractTableModel>
#include <QReadWriteLock>
#include <QList>

#include "sacndiscovery.h"


class sACNSourceInfo : public QObject
{
    Q_OBJECT
public:
    sACNSourceInfo(QString sourceName, QObject *parent = NULL);

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
    sACNDiscoveredSourceListModel(QObject *parent = NULL);
    ~sACNDiscoveredSourceListModel();

    int indexToUniverse(const QModelIndex &index);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;

public slots:

private slots:
    void newSource(QString cid);
    void expiredSource(QString cid);
    void newUniverse(QString cid, quint16 universe);
    void expiredUniverse(QString cid, quint16 universe);

private:
    mutable QReadWriteLock rwlock_ModelIndex;
    sACNDiscoveryRX *m_discoveryInstance;
    bool m_displayDDOnlySource;

    QHash<CID, sACNSourceInfo*> m_sources;
};

#endif // sACNDiscoveredSourceListModel_H
