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

#include "sacnuniverselistmodel.h"
#include "consts.h"
#include "ipaddr.h"
#include "preferences.h"
#include "sacnlistener.h"
#include "sacnsocket.h"
#include "streamcommon.h"
#include "streamingacn.h"
#include <QDebug>
#include <QNetworkInterface>

sACNUniverseInfo::sACNUniverseInfo(int u)
{
    universe = u;
}

sACNUniverseInfo::~sACNUniverseInfo()
{
    qDeleteAll(sources);
    sources.clear();
    sourcesByCid.clear();
}

sACNBasicSourceInfo::sACNBasicSourceInfo(sACNUniverseInfo * p)
    : universe(p->universe)
{}

sACNUniverseListModel::sACNUniverseListModel(QObject * parent)
    : QAbstractItemModel(parent)
{
    m_start = Preferences::Instance().GetUniversesListStart();

    m_displayDDOnlySource = Preferences::Instance().GetETCDisplayDDOnly();

    setStartUniverse(m_start);
}

void sACNUniverseListModel::setStartUniverse(int start)
{
    QMutexLocker datagram_locker(&mutex_readPendingDatagrams);
    QWriteLocker modelindex_locker(&rwlock_ModelIndex);

    // Limit max value
    const int startMax = (MAX_SACN_UNIVERSE - Preferences::Instance().GetUniversesListCount() + 1);
    if (start > startMax) start = startMax;

    beginResetModel();

    qDeleteAll(m_universes);
    m_universes.clear();

    // Copy listener sharedpointers, to release later
    QList<sACNManager::tListener> old_listeners = m_listeners;

    // Create listeners
    m_start = start;
    for (int universe = m_start; universe < m_start + Preferences::Instance().GetUniversesListCount(); universe++)
    {
        m_listeners.push_back(sACNManager::Instance().getListener(universe));

        m_universes << new sACNUniverseInfo(universe);

        // Add the existing sources
        for (int i = 0; i < m_listeners.back()->sourceCount(); i++)
        {
            modelindex_locker.unlock();
            sourceOnline(m_listeners.back()->source(i));
            modelindex_locker.relock();
        }

        connect(
            m_listeners.back().data(),
            &sACNListener::listenerStarted,
            this,
            &sACNUniverseListModel::listenerStarted);
        connect(m_listeners.back().data(), &sACNListener::sourceFound, this, &sACNUniverseListModel::sourceOnline);
        connect(m_listeners.back().data(), &sACNListener::sourceLost, this, &sACNUniverseListModel::sourceOffline);
        connect(m_listeners.back().data(), &sACNListener::sourceChanged, this, &sACNUniverseListModel::sourceChanged);
    }

    // Release old listener sharedpointers
    old_listeners.clear();

    endResetModel();
}

int sACNUniverseListModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && parent.internalPointer() == Q_NULLPTR)
    {
        if (parent.row() >= m_universes.count() || parent.row() < 0) return 0;
        return static_cast<int>(m_universes[parent.row()]->sources.count());
    }
    if (!parent.isValid())
    {
        return static_cast<int>(m_universes.count());
    }

    return 0;
}

int sACNUniverseListModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant sACNUniverseListModel::data(const QModelIndex & index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (index.parent().isValid())
        {
            sACNBasicSourceInfo * info = (sACNBasicSourceInfo *)index.internalPointer();
            return tr("%1 (%2)").arg(info->name).arg(info->address.toString());
        }
        else
        {
            auto universeIdx = index.row();
            int universe = universeIdx + m_start;
            auto listener = sACNManager::Instance().getListener(universe);

            QString displayString = tr("Universe %1").arg(universe);

            // Universe bind issues
            bool bindOk = (listener->getBindStatus().multicast != sACNRxSocket::BIND_FAILED);
            bindOk &= (listener->getBindStatus().unicast != sACNRxSocket::BIND_FAILED);

            if (bindOk)
                return QVariant(displayString);
            else
                return QVariant(displayString.append(QString(tr(" -- Interface Error"))));
        }
    }
    else if (role == Qt::EditRole)
    {
        return indexToUniverse(index);
    }
    return QVariant();
}

QModelIndex sACNUniverseListModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    if (!parent.isValid()) // This is a root item
        return createIndex(row, column);
    if (parent.isValid() && !parent.parent().isValid())
    {
        if (parent.row() >= m_universes.count() || parent.row() < 0) return QModelIndex();
        if (m_universes[parent.row()]->sources.count() > row)
        {
            return createIndex(row, column, m_universes[parent.row()]->sources.at(row));
        }
    }

    return QModelIndex();
}

QModelIndex sACNUniverseListModel::parent(const QModelIndex & index) const
{
    if (!index.isValid()) return QModelIndex();

    sACNBasicSourceInfo * i = static_cast<sACNBasicSourceInfo *>(index.internalPointer());
    if (i && rwlock_ModelIndex.tryLockForRead())
    {
        int parentRow = i->universe - m_start;
        QModelIndex ret = createIndex(parentRow, 0);

        rwlock_ModelIndex.unlock();
        return ret;
    }

    return QModelIndex();
}

void sACNUniverseListModel::listenerStarted(int universe)
{
    Q_UNUSED(universe);
    beginResetModel();
    endResetModel();
}

void sACNUniverseListModel::sourceOnline(sACNSource * source)
{
    QWriteLocker locker(&rwlock_ModelIndex);

    // In my display range?
    int univIndex = source->universe - m_start;
    if ((univIndex >= m_universes.count()) || (univIndex < 0))
    {
        return;
    }

    // Display sources that only transmit 0xdd?
    if (!m_displayDDOnlySource && !source->doing_dmx)
    {
        return;
    }

    // Create sACNBasicSourceInfo copy of sACNSource
    sACNBasicSourceInfo * info = Q_NULLPTR;
    info = new sACNBasicSourceInfo(m_universes[univIndex]);
    info->cid = source->src_cid;
    info->address = source->ip;
    info->name = source->name == Q_NULLPTR ? tr("????") : source->name;

    // Prevent duplicates
    if (m_universes[univIndex]->sourcesByCid.value(source->src_cid)) return;

    // We are adding the source for this universe
    QModelIndex parent = index(m_universes[univIndex]->universe - m_start, 0);
    const auto firstRow = static_cast<int>(m_universes[univIndex]->sources.count());
    const auto lastRow = firstRow;
    beginInsertRows(parent, firstRow, lastRow);
    m_universes[univIndex]->sources << info;
    m_universes[univIndex]->sourcesByCid[source->src_cid] = info;
    endInsertRows();
}

void sACNUniverseListModel::sourceChanged(sACNSource * source)
{
    QWriteLocker locker(&rwlock_ModelIndex);

    // In my display range?
    int univIndex = source->universe - m_start;
    if ((univIndex >= m_universes.count()) || (univIndex < 0))
    {
        return;
    }

    // Display sources that only transmit 0xdd?
    if (!m_displayDDOnlySource && !source->doing_dmx)
    {
        return;
    }

    // Update existing source
    sACNBasicSourceInfo * info = Q_NULLPTR;
    if (m_universes.count() + 1 < univIndex)
    {
        return;
    }
    info = m_universes[univIndex]->sourcesByCid.value(source->src_cid);
    if (info == Q_NULLPTR)
    {
        // Try to (re)add...
        locker.unlock();
        sourceOnline(source);
        sourceChanged(source);
        locker.relock();
        info = m_universes[univIndex]->sourcesByCid.value(source->src_cid);
        if (info == Q_NULLPTR)
        {
            return;
        }
    };
    info->address = source->ip;
    info->name = source->name;

    // Redraw entire universe
    const auto parent = index(static_cast<int>(m_universes[univIndex]->universe) - m_start, 0);
    QModelIndex topLeft = parent.sibling(0, 0);
    QModelIndex bottomRight = parent.sibling(static_cast<int>(m_universes[univIndex]->sources.count()), 0);
    emit dataChanged(topLeft, bottomRight);
}

void sACNUniverseListModel::sourceOffline(sACNSource * source)
{
    QWriteLocker locker(&rwlock_ModelIndex);
    int univIndex = source->universe - m_start;
    if ((univIndex >= m_universes.count()) || (univIndex < 0))
    {
        return;
    }

    // Remove existing source
    sACNBasicSourceInfo * info = Q_NULLPTR;
    info = m_universes[univIndex]->sourcesByCid.value(source->src_cid);
    if (info == Q_NULLPTR)
    {
        return;
    }

    QModelIndex parent = index(m_universes[univIndex]->universe - m_start, 0);
    const auto first = static_cast<int>(m_universes[univIndex]->sources.indexOf(info));
    int last = first;
    beginRemoveRows(parent, first, last);

    m_universes[univIndex]->sources.removeAll(info);
    m_universes[univIndex]->sourcesByCid.remove(source->src_cid);
    delete info;

    endRemoveRows();
}

int sACNUniverseListModel::indexToUniverse(const QModelIndex & index) const
{
    QReadLocker locker(&rwlock_ModelIndex);
    if (!index.isValid()) return 0;

    if (index.internalPointer() == Q_NULLPTR)
    {
        // root item
        return m_start + index.row();
    }

    sACNBasicSourceInfo * i = static_cast<sACNBasicSourceInfo *>(index.internalPointer());
    if (i)
    {
        return i->universe;
    }

    return 0;
}
