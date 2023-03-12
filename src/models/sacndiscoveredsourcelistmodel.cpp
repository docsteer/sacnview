// Copyright 2016 Tom Steer
// http://www.tomsteer.net
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sacndiscoveredsourcelistmodel.h"
#include "qcollator.h"
#include "sacndiscovery.h"
#include "preferences.h"
#include <QMutexLocker>
#include <QtDebug>


void sACNDiscoveredSourceListModel::modelContainer::addSource(const CID &cid)
{
    QMutexLocker locker(&listMutex);
    if (!sources.contains(cid))
    {
        sources.append(cid);
        universes.append(universeList_t());
        assert(sources.size() == universes.size());
    }
}

void sACNDiscoveredSourceListModel::modelContainer::removeSource(const CID &cid)
{
    QMutexLocker locker(&listMutex);

    const auto sourceRow = sources.indexOf(cid);
    if (sourceRow == -1)
        return;

    sources.removeAt(sourceRow);
    universes.removeAt(sourceRow);
    assert(sources.size() == universes.size());
}

void sACNDiscoveredSourceListModel::modelContainer::addUniverse(const CID &cid, quint16 universe)
{
    QMutexLocker locker(&listMutex);

    const auto sourceRow = sources.indexOf(cid);
    if (sourceRow == -1)
        return;

    if (!universes.at(sourceRow).contains(universe))
        universes[sourceRow].append(universe);
}

void sACNDiscoveredSourceListModel::modelContainer::removeUniverse(const CID &cid, quint16 universe)
{
    QMutexLocker locker(&listMutex);

    const auto sourceRow = sources.indexOf(cid);
    if (sourceRow == -1)
        return;

    const auto universeRow = universes.at(sourceRow).indexOf(universe);
    if (universeRow == -1)
        return;

    universes[sourceRow].removeAt(universeRow);
}

CID sACNDiscoveredSourceListModel::modelContainer::getCID(unsigned int row) const
{
    QMutexLocker locker(&listMutex);

    if (row >= static_cast<unsigned int>(sources.size()))
        return CID();

    return sources.at(row);
}

sACNDiscoveredSourceListModel::modelContainer::universe_t
    sACNDiscoveredSourceListModel::modelContainer::getUniverse(const CID &cid, unsigned int row) const
{
    QMutexLocker locker(&listMutex);

    if (!sources.contains(cid))
        return 0;

    const auto sourceRow = sources.indexOf(cid);

    if (row >= static_cast<unsigned int>(universes.size()))
        return 0;

    return universes.at(sourceRow).at(row);
}

int sACNDiscoveredSourceListModel::modelContainer::getRow(const CID &cid) const
{
    QMutexLocker locker(&listMutex);
    return sources.indexOf(cid);
}

int sACNDiscoveredSourceListModel::modelContainer::getRow(const CID &cid, universe_t universe) const
{
    QMutexLocker locker(&listMutex);
    const auto sourceRow = sources.indexOf(cid);
    if (sourceRow == -1)
        return -1;

    return universes.at(sourceRow).indexOf(universe);
}

size_t sACNDiscoveredSourceListModel::modelContainer::count() const
{
    QMutexLocker locker(&listMutex);
    return sources.count();
}

size_t sACNDiscoveredSourceListModel::modelContainer::count(const CID &cid) const
{
    QMutexLocker locker(&listMutex);

    const auto sourceRow = sources.indexOf(cid);
    if (sourceRow == -1)
        return 0;
    return universes.at(sourceRow).count();
}

sACNDiscoveredSourceListModel::sACNDiscoveredSourceListModel(QObject *parent) : QAbstractItemModel(parent),
    m_discoveryInstance(sACNDiscoveryRX::getInstance()),
    m_displayDDOnlySource(Preferences::Instance().GetETCDisplayDDOnly())
{
    // Add existing
    auto it = m_discoveryInstance->getDiscoveryList().constBegin();
    while (it != m_discoveryInstance->getDiscoveryList().constEnd())
        newSource(it.key());

    connect(m_discoveryInstance, SIGNAL(newSource(CID)), this, SLOT(newSource(CID)));
    connect(m_discoveryInstance, SIGNAL(expiredSource(CID)), this, SLOT(expiredSource(CID)));
    connect(m_discoveryInstance, SIGNAL(newUniverse(CID,quint16)), this, SLOT(newUniverse(CID,quint16)));
    connect(m_discoveryInstance, SIGNAL(expiredUniverse(CID,quint16)), this, SLOT(expiredUniverse(CID,quint16)));
}

void sACNDiscoveredSourceListModel::newSource(CID cid)
{
    if (m_sources.getRow(cid) == -1)
    {
        const auto childRow  = m_sources.count();
        const bool resetModel = (childRow == 0);
        if (resetModel)
            beginResetModel(); // Reset dummy help row
        else
            beginInsertRows(QModelIndex(), childRow, childRow);

        m_sources.addSource(cid);

        if (resetModel)
            endResetModel();
        else
            endInsertRows();
    }
}

void sACNDiscoveredSourceListModel::expiredSource(CID cid)
{
    qDebug() << "******************* Expired source" << CID::CIDIntoQString(cid);

    if(m_sources.getRow(cid) != -1)
    {
        const auto childRow = m_sources.getRow(cid);
        beginRemoveRows(QModelIndex(), childRow, childRow);
        m_sources.removeSource(cid);
        endRemoveRows();

        if (m_sources.count() == 0)
        {
            // Restore dummy help row
            beginResetModel();
            endResetModel();
        }
    }
}

void sACNDiscoveredSourceListModel::newUniverse(CID cid, quint16 universe)
{
    if (m_sources.getRow(cid) != -1)
    {
        const auto parentRow = m_sources.getRow(cid);
        auto parent = index(parentRow,0, QModelIndex());

        if (m_sources.getRow(cid, universe) == -1)
        {
            const auto childRow = m_sources.count(cid);
            beginInsertRows(parent, childRow, childRow);
            m_sources.addUniverse(cid, universe);
            endInsertRows();
        }
    }
}

void sACNDiscoveredSourceListModel::expiredUniverse(CID cid, quint16 universe)
{
    qDebug() << " Expired universe" << CID::CIDIntoQString(cid) << universe;

    if (m_sources.getRow(cid) != -1)
    {
        const auto parentRow = m_sources.getRow(cid);
        auto parent = index(parentRow,0, QModelIndex());
        const auto childRow = m_sources.getRow(cid, universe);

        if (childRow != -1)
        {
            beginRemoveRows(parent, childRow, childRow);
            m_sources.removeUniverse(cid, universe);
            endRemoveRows();
        }
    }
}

int sACNDiscoveredSourceListModel::indexToUniverse(const QModelIndex &index)
{
    if(!index.isValid())
        return 0;

    if(index.internalPointer()==Q_NULLPTR)
        // Source
        return 0;

    if (index.parent().isValid())
    {
        const auto cid = m_sources.getCID(index.parent().row());
        if (cid == CID())
            return 0;

        return m_sources.getUniverse(cid, index.row());
    }

    return 0;
}

int sACNDiscoveredSourceListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant sACNDiscoveredSourceListModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();

    // Details that no sources found
    if (!m_sources.count()) {
        if(role==Qt::DisplayRole)
        {
            return QVariant(tr("No discovery sources found"));
        } else if(role==Qt::ToolTipRole) {
            return QVariant(tr("Discovery relies upon\n"
                               "E1-31:2016 universe discovery packets\n"
                               "very few sources support this"));
        }
        return QVariant();
    }

    if(role==Qt::ToolTipRole)
    {
        if(index.parent().isValid())
        {
            return QVariant();
        }
        else
        {
            // Show CID as a tooltip
            const auto cid = m_sources.getCID(index.row());
            return QVariant(CID::CIDIntoQString(cid));
        }
    }
    if(role==Qt::DisplayRole)
    {
        if(index.parent().isValid())
        {
            // Display universes
            const auto cid = m_sources.getCID(index.parent().row());
            return QVariant(
                        QString("Universe %1")
                        .arg(m_sources.getUniverse(cid, index.row())));
        }
        else
        {
            // Source Name
            const auto cid = m_sources.getCID(index.row());
            return QVariant(m_discoveryInstance->getDiscoveryList().find(cid).value()->Name);
        }
    }

    if(role==Qt::UserRole && index.parent().isValid())
    {
        // Universe number
        const auto cid = m_sources.getCID(index.parent().row());
        return QVariant(m_sources.getUniverse(cid, index.row()));
    }
    return QVariant();
}

QModelIndex sACNDiscoveredSourceListModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!hasIndex(row, column, parent))
        return QModelIndex();
    if(!parent.isValid()) // This is a root item
        return createIndex(row, column);
    if(parent.isValid() && !parent.parent().isValid())
    {
        const auto cid = m_sources.getCID(parent.row());
        if (cid == CID())
            return QModelIndex();

        if (m_sources.count(cid) > row)
        {
            return createIndex(row, column, parent.row() + 1);
        }
    }

    return QModelIndex();
}

QModelIndex sACNDiscoveredSourceListModel::parent(const QModelIndex &index) const
{
    if(!index.isValid())
        return QModelIndex();

    if(index.internalId())
    {
        QModelIndex ret = createIndex(index.internalId() - 1, 0);
        return ret;
    }

    return QModelIndex();
}

int sACNDiscoveredSourceListModel::rowCount(const QModelIndex &parent) const
{
    // Sources count
    if(!parent.isValid())
    {
        if (!m_sources.count()) return 1; // Details that no sources found
        return m_sources.count();
    }

    // Universe count of source
    if(parent.isValid() && parent.internalPointer()==Q_NULLPTR)
    {
        const auto cid = m_sources.getCID(parent.row());
        if (cid == CID())
            return 0;
        return m_sources.count(cid);
    }

    return 0;
}


/******************************************* sACNDiscoveredSourceListProxy *********************************************/

sACNDiscoveredSourceListProxy::sACNDiscoveredSourceListProxy(QObject *parent) :
    QSortFilterProxyModel (parent),
    collator()
{
    // Used for sorting of the list
    collator.setNumericMode(true);
    collator.setCaseSensitivity(sortCaseSensitivity());
}

void sACNDiscoveredSourceListProxy::setSortCaseSensitivity(Qt::CaseSensitivity cs)
{
    collator.setCaseSensitivity(cs);
    QSortFilterProxyModel::setSortCaseSensitivity(cs);
}

bool sACNDiscoveredSourceListProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (sortColumn())
    {
        case 0: // Source Names
        {
            QString leftString = sourceModel()->data(left).toString();
            QString rightString = sourceModel()->data(right).toString();

            return collator.compare(leftString, rightString) < 0;
        }

        case 1: // Universe number
        {
            int leftUniverse = sourceModel()->data(left, Qt::UserRole).toInt();
            int rightUniverse = sourceModel()->data(right, Qt::UserRole).toInt();
            return leftUniverse < rightUniverse;
        }

        default:
            return false;
    }
}
