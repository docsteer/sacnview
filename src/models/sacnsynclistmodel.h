#ifndef sACNSyncListModel_H
#define sACNSyncListModel_H

#include <QObject>
#include <QMutex>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include "sacnsynchronization.h"

/**
 * @brief The sACNSyncListModel class provides a
 * QAbstractItemModel which represents all the synchronization addresses
 * with each address as a node with known synchronization sources as its children.
 */
class sACNSyncListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit sACNSyncListModel(QObject *parent = Q_NULLPTR);
     ~sACNSyncListModel();

    quint16 indexToUniverse(const QModelIndex &index);

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    class item;
    item *rootItem;
    QMutex itemMutex;

    void addRow(item *newItem);
    void removeRow(item *oldItem
                   );
    enum itemTypes {
        /*
         * itemType_Root
         * |_itemType_SyncAddress ["Sync Address x"]
         *      |_itemType_SyncAddress_Source ["Synchroniser"]
         *      |   |_itemType_SyncAddress_Source_CID ["CID: xxx-xxx-xxx..."]
         *      |       |_itemType_SyncAddress_Source_IP ["IP: x.x.x.x"]
         *      |       |_itemType_SyncAddress_Source_FPS ["FPS: nnHz"]
         *      |
         *      |_itemType_SyncAddress_Synced ["Synced Universes"]
         *          |_itemType_SyncAddress_Synced_Universe ["Universe n"]
         *              |_itemType_SyncAddress_Synced_Universe_Source ["mySourceName (x.x.x.x)"]
         */
        itemType_HelpMsg,
        itemType_Root,
        itemType_SyncAddress,
        itemType_SyncAddress_Source,
        itemType_SyncAddress_Source_CID,
        itemType_SyncAddress_Source_IP,
        itemType_SyncAddress_Source_FPS,
        itemType_SyncAddress_Synced,
        itemType_SyncAddress_Synced_Universe,
        itemType_SyncAddress_Synced_Universe_Source
    };

    class item : public QObject {
    public:
        item(item *parentItem, sACNSyncListModel *parentModel) :
            QObject(parentModel),
            m_parentModel(parentModel),
            m_parentItem(parentItem) {}
        virtual ~item();

    protected:
        item(item *parentItem) :
            QObject(parentItem->parentModel()),
            m_parentModel(parentItem->parentModel()),
            m_parentItem(parentItem) {}

    public:
        virtual itemTypes type() const { return itemType_Root; }

        int row() const;
        int column() const { return 0; }

        item *parent() const { return m_parentItem; }
        sACNSyncListModel *parentModel() const { return m_parentModel; }

        int childCount() const { return m_children.size(); }
        item *child(size_t index) const;
        const QVector<item *> &children() const { return m_children; }
        void addChild(item *);
        void removeChild(item *);

        virtual QVariant data(Qt::ItemDataRole role) const;

    protected:
        sACNSyncListModel *m_parentModel = Q_NULLPTR;
        item *m_parentItem  = Q_NULLPTR;
        QVector<item*> m_children = {};

    };

    class item_HelpMsg : public item {
    public:
        typedef QMap<Qt::ItemDataRole, QVariant> messageMap_t;
        item_HelpMsg(messageMap_t &messages, item *parent) :
            item(parent),
            m_messages(messages) {}

        itemTypes type() const override { return itemType_HelpMsg; }

        QVariant data(Qt::ItemDataRole role) const override;

    private:
        messageMap_t m_messages;
    };
    void addHelpMsg(item *parent);
    void RemoveHelpMsg(item *parent);

    class item_SyncAddress : public item {
    public:
        item_SyncAddress(tsyncAddress syncAddress, item *parent) :
            item(parent),
            m_syncAddress(syncAddress) {}

        itemTypes type() const override { return itemType_SyncAddress; }
        tsyncAddress syncAddress() const { return m_syncAddress; }

        QVariant data(Qt::ItemDataRole role) const override;

    private:
        tsyncAddress m_syncAddress = 0;
    };
    struct itemPair_SyncAddress {
        item_SyncAddress* actualItem = Q_NULLPTR;
        item* parentItem = Q_NULLPTR;
    };
    itemPair_SyncAddress getSyncAddress(tsyncAddress syncAddress);
    bool isSyncAddressEmpty(tsyncAddress syncAddress);
    void addSyncAddress(tsyncAddress syncAddress);
    void removeSyncAddress(tsyncAddress syncAddress);

    class item_SyncAddress_Source: public item {
    public:
        item_SyncAddress_Source(item *parent) :
            item(parent, parent->parentModel()) {}

        itemTypes type() const override { return itemType_SyncAddress_Source; }

        QVariant data(Qt::ItemDataRole role) const override;
    };

    class item_SyncAddress_Source_CID : public item {
    public:
        item_SyncAddress_Source_CID(const CID &cid, item *parent) :
            item(parent),
            m_cid(cid) {}

        itemTypes type() const override { return itemType_SyncAddress_Source_CID; }
        CID cid() const { return m_cid; }

        QVariant data(Qt::ItemDataRole role) const override;

    private:
        CID m_cid;
    };
    struct itemPair_SyncAddress_Source_CID {
        item_SyncAddress_Source_CID* actualItem = Q_NULLPTR;
        item* parentItem = Q_NULLPTR;
    };
    itemPair_SyncAddress_Source_CID getSyncSourceCID(tsyncAddress syncAddress, CID cid);
    void addSyncSourceCID(tsyncAddress syncAddress, CID cid);
    void removeSyncSourceCID(tsyncAddress syncAddress, CID cid);

    class item_SyncAddress_Source_IP : public item {
    public:
        item_SyncAddress_Source_IP(const QHostAddress &address, item *parent) :
            item(parent),
            m_address(address) {}

        itemTypes type() const override { return itemType_SyncAddress_Source_IP; }
        QHostAddress address() const { return m_address; }

        QVariant data(Qt::ItemDataRole role) const override;

    private:
          QHostAddress m_address;
    };

    class item_SyncAddress_Source_FPS : public item {
    public:
        item_SyncAddress_Source_FPS(FpsCounter *fps, item *parent);

        itemTypes type() const override { return itemType_SyncAddress_Source_FPS; }

        QVariant data(Qt::ItemDataRole role) const override;

    private:
        FpsCounter *m_fps;
    };

    class item_SyncAddress_Synced: public item {
    public:
        item_SyncAddress_Synced(item *parent) :
            item(parent, parent->parentModel()) {}

        itemTypes type() const override { return itemType_SyncAddress_Synced; }

        QVariant data(Qt::ItemDataRole role) const override;
    };

    class item_SyncAddress_Synced_Universe : public item {
    public:
        item_SyncAddress_Synced_Universe(quint16 universe, item *parent) :
            item(parent),
            m_universe(universe) {}

        itemTypes type() const override { return itemType_SyncAddress_Synced_Universe; }
        quint16 universe() const { return m_universe; }

        QVariant data(Qt::ItemDataRole role) const override;

    private:
        quint16 m_universe = 0;
    };
    struct itemPair_SyncAddress_Synced_Universe {
        item_SyncAddress_Synced_Universe* actualItem = Q_NULLPTR;
        item* parentItem = Q_NULLPTR;
    };
    itemPair_SyncAddress_Synced_Universe getSyncedUniverse(tsyncAddress syncAddress, quint16 universe);
    void addSyncedUniverse(tsyncAddress syncAddress, quint16 universe);
    void removeSyncedUniverse(tsyncAddress syncAddress, quint16 universe);

    class item_SyncAddress_Synced_Universe_Source : public item {
    public:
        item_SyncAddress_Synced_Universe_Source(const sACNSource *source, item *parent) :
            item(parent),
            m_source(source) {}

        itemTypes type() const override { return itemType_SyncAddress_Synced_Universe_Source; }
        const sACNSource *source() const { return m_source; }
        CID cid() const { return m_source->src_cid; }

        QVariant data(Qt::ItemDataRole role) const override;

    private:
          const sACNSource *m_source = Q_NULLPTR;
    };
    struct itemPair_SyncAddress_Synced_Universe_Source {
        item_SyncAddress_Synced_Universe_Source* actualItem = Q_NULLPTR;
        item* parentItem = Q_NULLPTR;
    };
    itemPair_SyncAddress_Synced_Universe_Source getSyncedUniverseSource(const sACNSource *source);
    void addSyncedUniverseSource(const sACNSource *source);
    void removeSyncedUniverseSource(const sACNSource *source);

public:
    class proxy : public QSortFilterProxyModel
    {
    friend class sACNSyncListModel;

    public:
        proxy(QObject *parent = Q_NULLPTR) : QSortFilterProxyModel(parent) {};

    protected:
        virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    };
};



#endif // sACNSyncListModel_H
