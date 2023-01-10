#include "sacnsynclistmodel.h"
#include "sacnsynchronization.h"
#include "CID.h"
#include <QtDebug>

sACNSyncListModel::sACNSyncListModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    beginResetModel();
    rootItem = new item(Q_NULLPTR, this);
    addHelpMsg(rootItem);
    endResetModel();

    // Existing Sync Addresses
    for (auto syncAddress : sACNSynchronizationRX::getInstance()->getSynchronizationAddresses())
        addSyncAddress(syncAddress);
    // -- Source with no known Synchroniser
    for (auto weakListener : sACNManager::Instance().getListenerList()) {
        auto listener = weakListener.toStrongRef();
        for (auto source : listener->getSourceList())
            if (!source->active.Expired() &&
                    source->synchronization)
                addSyncAddress(source->synchronization);
    }

    // Future Sync Addresses
    connect (sACNSynchronizationRX::getInstance(), &sACNSynchronizationRX::newSyncAddress,
             this, &sACNSyncListModel::addSyncAddress);
    // -- Source with no known Synchroniser
    connect (&sACNManager::Instance(), &sACNManager::sourceFound,
             this, [=](quint16 universe, sACNSource *source)
    {
        Q_UNUSED(universe);
        if (source->synchronization) {
            addSyncAddress(source->synchronization);
        }
    });
    connect (&sACNManager::Instance(), &sACNManager::sourceResumed,
             this, [=](quint16 universe, sACNSource *source)
    {
        Q_UNUSED(universe);
        if (source->synchronization) {
            addSyncAddress(source->synchronization);
        }
    });

    // Expired Sync Addresses
    connect (sACNSynchronizationRX::getInstance(), &sACNSynchronizationRX::expiredSyncAddress,
             this, [=](tsyncAddress syncAddress)
    {
        if (isSyncAddressEmpty(syncAddress))
            removeSyncAddress(syncAddress);
    }, Qt::DirectConnection);
    // -- Source
    connect (&sACNManager::Instance(), &sACNManager::sourceLost,
             this, [=](quint16 universe, sACNSource *source)
    {
        Q_UNUSED(universe);
        if (isSyncAddressEmpty(source->synchronization))
            removeSyncAddress(source->synchronization);
    });
}

sACNSyncListModel::~sACNSyncListModel() {
    delete rootItem;
}

quint16 sACNSyncListModel::indexToUniverse(const QModelIndex &index)
{
    if(!index.isValid())
        return 0;

    bool ok = false;
    quint16 universe = index.data(sACNSyncListModel::Roles::Universe).toUInt(&ok);
    if (ok)
        return universe;
    else
        return 0;
}

QVariant sACNSyncListModel::data(const QModelIndex &index, int role) const {

    if (!index.isValid())
        return QVariant();

    auto *item = static_cast<class item*>(index.internalPointer());

    return item->data(static_cast<Qt::ItemDataRole>(role));
}

Qt::ItemFlags sACNSyncListModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QModelIndex sACNSyncListModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    item *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<item*>(parent.internalPointer());

    auto *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex sACNSyncListModel::parent(const QModelIndex &index) const {
    if (!index.isValid())
        return QModelIndex();

    auto *childItem = static_cast<item*>(index.internalPointer());
    if (!childItem)
        return QModelIndex();

    item *parentItem = childItem->parent();
    if (!parentItem)
        return QModelIndex();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), parentItem->column(), parentItem);
}

int sACNSyncListModel::rowCount(const QModelIndex &parent) const {
    item *parentItem;
    if (parent.column() > columnCount())
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<item*>(parent.internalPointer());

    return parentItem->childCount();
}

int sACNSyncListModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return 1;
}

void sACNSyncListModel::addRow(item *newItem) {  
    itemMutex.lock();

    item *parentItem = newItem->parent()
            ? newItem->parent()
            : rootItem;

    QModelIndex parent = parentItem != rootItem
            ? createIndex(parentItem->row(), parentItem->column(), parentItem)
            : QModelIndex();

    auto row = parentItem->childCount();
    beginInsertRows(parent, row, row);
//    qDebug() << "sACNSyncListModel::addRow" << "Row" << row
//             << newItem->data(Qt::DisplayRole).toString()
//             << "Parent" << parentItem->data(Qt::DisplayRole).toString();
    parentItem->addChild(newItem);
    endInsertRows();

    itemMutex.unlock();

    // Help message management
    if (newItem->type() != itemType_HelpMsg) {
        RemoveHelpMsg(parentItem);
        addHelpMsg(newItem);
    }
}

void sACNSyncListModel::removeRow(item *oldItem) {
    itemMutex.lock();

    item *parentItem = oldItem->parent()
            ? oldItem->parent()
            : rootItem;

    QModelIndex parent = parentItem != rootItem
            ? createIndex(parentItem->row(), parentItem->column(), parentItem)
            : QModelIndex();

    auto row = oldItem->row();
    beginRemoveRows(parent, row, row);
//    qDebug() << "sACNSyncListModel::removeRow" << "Row" << row
//             << oldItem->data(Qt::DisplayRole).toString()
//             << "Parent" << parentItem->data(Qt::DisplayRole).toString();
    parentItem->removeChild(oldItem);
    endRemoveRows();

    itemMutex.unlock();

    // Help message management
    if (parentItem->type() != itemType_HelpMsg
            && !parentItem->childCount())
        addHelpMsg(parentItem);
}


// ---------- Item ----------
sACNSyncListModel::item::~item() {
    qDeleteAll(m_children);
}

int sACNSyncListModel::item::row() const {
    if (m_parentItem)
        return m_parentItem->m_children.indexOf(const_cast<item*>(this));

    return 0;
}

sACNSyncListModel::item *sACNSyncListModel::item::child(size_t index) const {
    if (index > static_cast<size_t>(m_children.size()))
        return Q_NULLPTR;

    return m_children.at(static_cast<int>(index));
}

void sACNSyncListModel::item::addChild(item* newItem) {
    m_children.append(newItem);
}

void sACNSyncListModel::item::removeChild(item* oldItem) {
    m_children.removeOne(oldItem);
    oldItem->deleteLater();
}

QVariant sACNSyncListModel::item::data(Qt::ItemDataRole role) const {
    Q_UNUSED(role);
    return QVariant();
}

// ---------- item_HelpMsg ----------
void sACNSyncListModel::addHelpMsg(item *parent) {
    if (!parent)
        return;

    item_HelpMsg::messageMap_t messages;
    switch (parent->type()) {
        case itemType_Root:
            messages.insert(Qt::DisplayRole,
                            QVariant(tr("No known details")));
            messages.insert(Qt::ToolTipRole,
                            QVariant(tr("Synchronisation relies upon\n"
                                        "E1-31:2016 universe synchronisation packets\n"
                                        "not all sources support or require this")));
            break;

        case itemType_SyncAddress_Source:
            messages.insert(Qt::DisplayRole,
                            QVariant(tr("No known sources")));
            messages.insert(Qt::ToolTipRole,
                            QVariant(tr("Synchronisation provides no discovery method\n"
                                        "A universe listener must already be present")));
            break;

        case itemType_SyncAddress_Synced:
            messages.insert(Qt::DisplayRole,
                            QVariant(tr("No known universes")));
            messages.insert(Qt::ToolTipRole,
                            QVariant(tr("Synchronisation provides no discovery method\n"
                                        "A universe listener must already be present")));
            break;

        case itemType_HelpMsg:
        default: return;
    }

    addRow(new item_HelpMsg(messages, parent));
}

void sACNSyncListModel::RemoveHelpMsg(item *parent) {
    if (!parent)
        return;

    for (auto child : parent->children())
        if (child->type() == itemType_HelpMsg)
            removeRow(child);
}

QVariant sACNSyncListModel::item_HelpMsg::data(Qt::ItemDataRole role) const {
    return m_messages.value(role);
}

// ---------- item_SyncAddress ----------
sACNSyncListModel::itemPair_SyncAddress sACNSyncListModel::getSyncAddress(tsyncAddress syncAddress) {
    itemPair_SyncAddress ret;

    // Locate parent
    ret.parentItem = rootItem;

    // Check for existing
    for (auto child : ret.parentItem->children())
        if (child->type() == itemType_SyncAddress)
            if (static_cast<item_SyncAddress*>(child)->syncAddress() == syncAddress)
                ret.actualItem = static_cast<item_SyncAddress*>(child);

    return ret;
}

bool sACNSyncListModel::isSyncAddressEmpty(tsyncAddress syncAddress) {
    // Check Synced universes
    for (auto weakListener : sACNManager::Instance().getListenerList()) {
        auto listener = weakListener.toStrongRef();
        for (auto source : listener->getSourceList())
            if (!source->active.Expired() &&
                    source->synchronization == syncAddress)
                return false;
    }

    // Check Synchronisers
    return !sACNSynchronizationRX::getInstance()->getSynchronizationAddresses().contains(syncAddress);
}

void sACNSyncListModel::addSyncAddress(tsyncAddress syncAddress) {
    auto itemPair = getSyncAddress(syncAddress);

    // Check existing
    if (itemPair.actualItem)
        return;

    // Create new
    if (itemPair.parentItem) {
        auto newItem = new item_SyncAddress(syncAddress, itemPair.parentItem);
        addRow(newItem);
        addRow(new item_SyncAddress_Source(newItem));
        addRow(new item_SyncAddress_Synced(newItem));
    }

    // Existing Synced universes
    for (auto weakListener : sACNManager::Instance().getListenerList()) {
        auto listener = weakListener.toStrongRef();
        for (auto source : listener->getSourceList())
            if (!source->active.Expired() &&
                    source->synchronization == syncAddress) {
                addSyncedUniverse(syncAddress, source->universe);
                addSyncedUniverseSource(source);
            }
    }

    // Future Synced universes
    connect (&sACNManager::Instance(), &sACNManager::sourceFound,
             this, [=](quint16 universe, sACNSource *source)
    {
        if (source->synchronization == syncAddress) {
            addSyncedUniverse(syncAddress, universe);
            addSyncedUniverseSource(source);
        }
    });
    connect (&sACNManager::Instance(), &sACNManager::sourceChanged,
             this, [=](quint16 universe, sACNSource *source)
    {
        if (source->synchronization == syncAddress) {
            addSyncedUniverse(syncAddress, universe);
            addSyncedUniverseSource(source);
        }
    });
    connect (&sACNManager::Instance(), &sACNManager::sourceResumed,
             this, [=](quint16 universe, sACNSource *source)
    {
        if (source->synchronization == syncAddress) {
            addSyncedUniverse(syncAddress, universe);
            addSyncedUniverseSource(source);
        }
    });


    // Expired Synced universes
    connect (&sACNManager::Instance(), &sACNManager::sourceLost,
             this, [=](quint16 universe, sACNSource *source)
    {
        if (source->synchronization == syncAddress) {
            removeSyncedUniverseSource(source);
            if (getSyncedUniverse(syncAddress, universe).actualItem &&
                    !getSyncedUniverse(syncAddress, universe).actualItem->childCount())
                removeSyncedUniverse(syncAddress, universe);
        }
    });

    // Existing Sync sources
    for (auto syncSourceCID : sACNSynchronizationRX::getInstance()->getSynchronizationSources(syncAddress).keys())
        addSyncSourceCID(syncAddress, syncSourceCID);

    // Future Sync sources
    connect(sACNSynchronizationRX::getInstance(), &sACNSynchronizationRX::newSource,
            this, &sACNSyncListModel::addSyncSourceCID,
            Qt::DirectConnection);

    // Expired Sync sources
    connect(sACNSynchronizationRX::getInstance(), &sACNSynchronizationRX::expiredSource,
            this, &sACNSyncListModel::removeSyncSourceCID,
            Qt::DirectConnection);
}

void sACNSyncListModel::removeSyncAddress(tsyncAddress syncAddress) {
    auto item = getSyncAddress(syncAddress).actualItem;
    if (!item)
        return;

    removeRow(item);
}

QVariant sACNSyncListModel::item_SyncAddress::data(Qt::ItemDataRole role) const {
    switch (role) {
        case Qt::DisplayRole:
            return QVariant(tr("Sync Address %1").arg(syncAddress()));

        default:
            return QVariant();
    }
}

// ---------- item_SyncAddress_Source ----------
QVariant sACNSyncListModel::item_SyncAddress_Source::data(Qt::ItemDataRole role) const {
    switch (role) {
        case Qt::DisplayRole:
            return QVariant(tr("Synchroniser"));

        default:
            return QVariant();
    }
}

// ---------- item_SyncAddress_Source_CID ----------
sACNSyncListModel::itemPair_SyncAddress_Source_CID sACNSyncListModel::getSyncSourceCID(tsyncAddress syncAddress, CID cid) {
    itemPair_SyncAddress_Source_CID ret;

    // Locate parent
    auto itemSyncAddress = getSyncAddress(syncAddress).actualItem;
    if (!itemSyncAddress)
        return ret;

    for (const auto child : itemSyncAddress->children())
        if (child->type() == itemType_SyncAddress_Source)
            ret.parentItem = child;
    if (!ret.parentItem)
        return ret;

    // Check for existing
    for (auto child : ret.parentItem->children())
        if (child->type() == itemType_SyncAddress_Source_CID)
            if (static_cast<item_SyncAddress_Source_CID*>(child)->cid() == cid)
                ret.actualItem = static_cast<item_SyncAddress_Source_CID*>(child);

    return ret;
}

void sACNSyncListModel::addSyncSourceCID(tsyncAddress syncAddress, CID cid) {
    auto itemPair = getSyncSourceCID(syncAddress, cid);

    // Check existing
    if (itemPair.actualItem)
        return;

    // Create new
    if (itemPair.parentItem) {
        auto source = sACNSynchronizationRX::getInstance()->getSynchronizationSources(syncAddress).value(cid);
        auto CIDItem = new item_SyncAddress_Source_CID(cid, itemPair.parentItem);
        addRow(CIDItem);
        addRow(new item_SyncAddress_Source_IP(source.sender, CIDItem));
        addRow(new item_SyncAddress_Source_FPS(source.fps, CIDItem));
    }
}

void sACNSyncListModel::removeSyncSourceCID(tsyncAddress syncAddress, CID cid) {
    auto item = getSyncSourceCID(syncAddress, cid).actualItem;
    if (!item)
        return;

    removeRow(item);
}

QVariant sACNSyncListModel::item_SyncAddress_Source_CID::data(Qt::ItemDataRole role) const {
    switch (role) {
        case Qt::DisplayRole:
            return QVariant(QString("CID: %1").arg(m_cid));

        default:
            return QVariant();
    }
}

// ---------- item_SyncAddress_Source_IP ----------
QVariant sACNSyncListModel::item_SyncAddress_Source_IP::data(Qt::ItemDataRole role) const {
    switch (role) {
        case Qt::DisplayRole:
            return QVariant(QString("IP: %1").arg(m_address.toString()));

        default:
            return QVariant();
    }
}

// ---------- item_SyncAddress_Source_FPS ----------
sACNSyncListModel::item_SyncAddress_Source_FPS::item_SyncAddress_Source_FPS(FpsCounter *fps, item *parent) :
    item(parent),
    m_fps(fps)
{
    connect(m_fps, &FpsCounter::updatedFPS,
            this, [=]()
    {
        auto parentModel = parent->parentModel();
        if (parent->parentModel()) {
            QModelIndex index = parentModel->createIndex(row(), column(), parent->child(row()));
            emit parentModel->dataChanged(index, index, {Qt::DisplayRole});
        }
    });
}

QVariant sACNSyncListModel::item_SyncAddress_Source_FPS::data(Qt::ItemDataRole role) const {
    switch (role) {
        case Qt::DisplayRole:
            return QVariant(QString("FPS: %1").arg(m_fps->FPS()));

        default:
            return QVariant();
    }
}

// ---------- item_syncAddress_Synced ----------
QVariant sACNSyncListModel::item_SyncAddress_Synced::data(Qt::ItemDataRole role) const {
    switch (role) {
        case Qt::DisplayRole:
            return QVariant(tr("Synced Universes"));

        default:
            return QVariant();
    }
}

// ---------- item_SyncAddress_Synced_Universe ----------
sACNSyncListModel::itemPair_SyncAddress_Synced_Universe sACNSyncListModel::getSyncedUniverse(tsyncAddress syncAddress, quint16 universe) {
    itemPair_SyncAddress_Synced_Universe ret;

    // Locate parent
    auto itemSyncAddress = getSyncAddress(syncAddress).actualItem;
    if (!itemSyncAddress)
        return ret;

    for (const auto child : itemSyncAddress->children())
        if (child->type() == itemType_SyncAddress_Synced)
            ret.parentItem = child;
    if (!ret.parentItem)
        return ret;

    // Check for existing
    for (auto child : ret.parentItem->children())
        if (child->type() == itemType_SyncAddress_Synced_Universe)
            if (static_cast<item_SyncAddress_Synced_Universe*>(child)->universe() == universe)
                ret.actualItem = static_cast<item_SyncAddress_Synced_Universe*>(child);

    return ret;
}

void sACNSyncListModel::addSyncedUniverse(tsyncAddress syncAddress, quint16 universe) {
    auto itemPair = getSyncedUniverse(syncAddress, universe);

    // Check existing
    if (itemPair.actualItem)
        return;

    // Create new
    if (itemPair.parentItem)
        addRow(new item_SyncAddress_Synced_Universe(universe, itemPair.parentItem));
}

void sACNSyncListModel::removeSyncedUniverse(tsyncAddress syncAddress, quint16 universe) {
    auto item = getSyncedUniverse(syncAddress, universe).actualItem;
    if (!item)
        return;

    removeRow(item);
}

QVariant sACNSyncListModel::item_SyncAddress_Synced_Universe::data(Qt::ItemDataRole role) const {
    switch (role) {
        case Qt::DisplayRole:
            return QVariant(tr("Universe %1").arg(universe()));

        case static_cast<Qt::ItemDataRole>(Roles::Universe):
            return universe();

        default:
            return QVariant();
    }
}

// ---------- item_SyncAddress_Synced_Universe_Source ----------
sACNSyncListModel::itemPair_SyncAddress_Synced_Universe_Source sACNSyncListModel::getSyncedUniverseSource(const sACNSource *source) {
    itemPair_SyncAddress_Synced_Universe_Source ret;

    // Locate parent
    ret.parentItem = getSyncedUniverse(source->synchronization, source->universe).actualItem;
    if (!ret.parentItem)
        return ret;

    // Check for existing
    for (auto child : ret.parentItem->children())
        if (child->type() == itemType_SyncAddress_Synced_Universe_Source)
            if (static_cast<item_SyncAddress_Synced_Universe_Source*>(child)->cid() == source->src_cid)
                ret.actualItem = static_cast<item_SyncAddress_Synced_Universe_Source*>(child);

    return ret;
}

void sACNSyncListModel::addSyncedUniverseSource(const sACNSource *source) {
    auto itemPair = getSyncedUniverseSource(source);

    // Check existing
    if (itemPair.actualItem)
        return;

    // Create new
    if (itemPair.parentItem)
        addRow(new item_SyncAddress_Synced_Universe_Source(source, itemPair.parentItem));
}

void sACNSyncListModel::removeSyncedUniverseSource(const sACNSource *source) {
    auto item = getSyncedUniverseSource(source).actualItem;
    if (!item)
        return;

    removeRow(item);
}

QVariant sACNSyncListModel::item_SyncAddress_Synced_Universe_Source::data(Qt::ItemDataRole role) const {
    switch (role) {
        case Qt::DisplayRole:
             if (m_source) {
                 return (QString("%1 (%2)").arg(m_source->name).arg(m_source->ip.toString()));
             } return QVariant();

         case Qt::ToolTipRole:
             if (m_source) {
                 return QVariant(QString("CID: %1").arg(m_source->src_cid));
             } return QVariant();

        case static_cast<Qt::ItemDataRole>(Roles::Universe):
            return m_source->universe;

         default:
             return QVariant();
    }
}


// ---------- proxy ----------

bool sACNSyncListModel::proxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftModel = static_cast<sACNSyncListModel::item*>(left.internalPointer());
    auto rightModel = static_cast<sACNSyncListModel::item*>(right.internalPointer());
    if (!leftModel || !rightModel)
        return false;

    if (leftModel->type() != rightModel->type())
        return leftModel->type() < rightModel->type();  // Sort by type enum

    switch (leftModel->type()) {
        case itemType_SyncAddress: // Sort by syncaddress
            return static_cast<item_SyncAddress*>(leftModel)->syncAddress()
                    < static_cast<item_SyncAddress*>(rightModel)->syncAddress();

        case itemType_SyncAddress_Source_CID: // Sort by CID
            return static_cast<item_SyncAddress_Source_CID*>(leftModel)->cid()
                    < static_cast<item_SyncAddress_Source_CID*>(rightModel)->cid();

        case itemType_SyncAddress_Synced_Universe: // Sort by Universe number
            return static_cast<item_SyncAddress_Synced_Universe*>(leftModel)->universe()
                    < static_cast<item_SyncAddress_Synced_Universe*>(rightModel)->universe();

        case itemType_SyncAddress_Synced_Universe_Source: // Sort by display name
            return static_cast<item_SyncAddress_Synced_Universe_Source*>(leftModel)->data(Qt::DisplayRole).toString()
                    < static_cast<item_SyncAddress_Synced_Universe_Source*>(rightModel)->data(Qt::DisplayRole).toString();

        // Don't sort/can't sort
        case itemType_HelpMsg:
        case itemType_Root:
        case itemType_SyncAddress_Source_IP:
        case itemType_SyncAddress_Source_FPS:
        case itemType_SyncAddress_Source:
        case itemType_SyncAddress_Synced:
            return false;
    }

    return false;
}
