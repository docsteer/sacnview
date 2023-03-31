#ifndef sACNDiscoveredSourceListModel_H
#define sACNDiscoveredSourceListModel_H

#include <QObject>
#include <QAbstractTableModel>
#include <QReadWriteLock>
#include <QList>
#include <QSortFilterProxyModel>

#include "qcollator.h"
#include "sacndiscovery.h"

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

    class modelContainer
    {
    public:
        typedef quint16 universe_t;

        void addSource(const CID &cid);
        void removeSource(const CID &cid);

        void addUniverse(const CID &cid, universe_t universe);
        void removeUniverse(const CID &cid, universe_t universe);

        CID getCID(unsigned int row) const;
        universe_t getUniverse(const CID &cid, unsigned int row) const;

        int getRow(const CID &cid) const;
        int getRow(const CID &cid, universe_t universe) const;

        /**
         * @brief Count
         * @return Number of sources
         */
        qsizetype count() const;

        /**
         * @brief Count
         * @param cid Source
         * @return Number of universes for source
         */
        qsizetype count(const CID &cid) const;


    private:
         mutable QMutex listMutex;

        QList<CID> sources; // List of sources CIDs
        typedef QList<universe_t> universeList_t;
        QList<universeList_t> universes; // List of universes for source, key is m_sources row
    };

    modelContainer m_sources;
};

/**
 * @brief The sACNDiscoveredSourceListProxy class provides a
 * model which sorts and filters the raw sACNDiscoveredSourceListModel.
 * Currently it just sorts items
 */
class sACNDiscoveredSourceListProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    sACNDiscoveredSourceListProxy(QObject *parent = Q_NULLPTR);

    void setSortCaseSensitivity(Qt::CaseSensitivity cs);

protected:
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QCollator collator;
};

#endif // sACNDiscoveredSourceListModel_H
