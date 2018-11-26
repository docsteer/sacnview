// Copyright 2016 Tom Barthel-Steer
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
#include "sacndiscovery.h"
#include "streamingacn.h"
#include "streamcommon.h"
#include "ipaddr.h"
#include "preferences.h"
#include "sacnsocket.h"
#include "consts.h"
#include "preferences.h"

#include <QDebug>
#include <QNetworkInterface>

sACNSourceInfo::sACNSourceInfo(QString sourceName, QObject *parent): QObject(parent),
    name(sourceName)
{}


sACNDiscoveredSourceListModel::sACNDiscoveredSourceListModel(QObject *parent) : QAbstractItemModel(parent),
    m_discoveryInstance(sACNDiscoveryRX::getInstance()),
    m_displayDDOnlySource(Preferences::getInstance()->GetDisplayDDOnly())
{
    connect(m_discoveryInstance, SIGNAL(newSource(QString)), this, SLOT(newSource(QString)));
    connect(m_discoveryInstance, SIGNAL(expiredSource(QString)), this, SLOT(expiredSource(QString)));
    connect(m_discoveryInstance, SIGNAL(newUniverse(QString,quint16)), this, SLOT(newUniverse(QString,quint16)));
    connect(m_discoveryInstance, SIGNAL(expiredUniverse(QString,quint16)), this, SLOT(expiredUniverse(QString,quint16)));
}

sACNDiscoveredSourceListModel::~sACNDiscoveredSourceListModel()
{
    qDeleteAll(m_sources);
    m_sources.clear();
}

void sACNDiscoveredSourceListModel::newSource(QString cid)
{
    sACNDiscoveryRX::tDiscoveryList discoveryList = m_discoveryInstance->getDiscoveryList();
    CID cidObj = CID::StringToCID(cid.toLatin1().data());

    if (discoveryList.contains(cidObj) && !m_sources.contains(cidObj))
    {
        sACNSourceInfo *newSource = new sACNSourceInfo(discoveryList[cidObj]->Name);
        m_sources[cidObj] = newSource;
        beginInsertRows(
                    QModelIndex(),
                    m_sources.keys().indexOf(cidObj),
                    m_sources.keys().indexOf(cidObj));
        endInsertRows();
    }
}

void sACNDiscoveredSourceListModel::expiredSource(QString cid)
{
    sACNDiscoveryRX::tDiscoveryList discoveryList = m_discoveryInstance->getDiscoveryList();
    CID cidObj = CID::StringToCID(cid.toLatin1().data());

    if (discoveryList.contains(cidObj) && !m_sources.contains(cidObj))
    {
        int childRow = m_sources.keys().indexOf(cidObj);
        beginRemoveRows(
                    index(0, 0, QModelIndex()),
                    childRow,
                    childRow);
        m_sources.remove(cidObj);
        endRemoveRows();
    }
}

void sACNDiscoveredSourceListModel::newUniverse(QString cid, quint16 universe)
{
    sACNDiscoveryRX::tDiscoveryList discoveryList = m_discoveryInstance->getDiscoveryList();
    CID cidObj = CID::StringToCID(cid.toLatin1().data());

    if (discoveryList.contains(cidObj) && m_sources.contains(cidObj))
    {
        m_sources.value(cidObj)->universes.append(universe);
        int parentRow = m_sources.keys().indexOf(cidObj);

        beginResetModel();
        qSort(m_sources.value(cidObj)->universes.begin(), m_sources.value(cidObj)->universes.end());
        endResetModel();
//        beginRemoveRows(
//            index(parentRow, 0, QModelIndex()),
//            0,
//            m_sources.count());
//        endRemoveRows();
//        beginInsertRows(
//                    index(parentRow, 0, QModelIndex()),
//                    m_sources.value(cidObj)->universes.indexOf(universe),
//                    m_sources.value(cidObj)->universes.indexOf(universe));
//        endInsertRows();
    }
}

void sACNDiscoveredSourceListModel::expiredUniverse(QString cid, quint16 universe)
{
    sACNDiscoveryRX::tDiscoveryList discoveryList = m_discoveryInstance->getDiscoveryList();
    CID cidObj = CID::StringToCID(cid.toLatin1().data());

    if (discoveryList.contains(cidObj) && m_sources.contains(cidObj))
    {
        int childRow = m_sources.value(cidObj)->universes.indexOf(universe);
        int parentRow = m_sources.keys().indexOf(cidObj);
        beginRemoveRows(
                    index(parentRow, 0, QModelIndex()),
                    childRow,
                    childRow);
        m_sources.value(cidObj)->universes.removeAll(universe);
        endRemoveRows();
    }
}

int sACNDiscoveredSourceListModel::indexToUniverse(const QModelIndex &index)
{
    if(!index.isValid())
        return 0;

    if(index.internalPointer()==NULL)
        // Source
        return 0;

    if (index.parent().isValid())
    {
        auto sourceIdx = index.parent().row();
        auto universeIdx = index.row();
        auto cid = m_sources.keys().at(sourceIdx);
        return m_sources.value(cid)->universes.value(universeIdx);
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
    if(role==Qt::DisplayRole)
    {
        if(index.parent().isValid())
        {
            // Display universes
            QString universeString = QString("Universe %1")
                    .arg(m_sources.values()[index.parent().row()]->universes.at(index.row()));
            return QVariant(universeString);
        }
        else
        {
            // Display sources
            QString sourceString = QString("%1\n(%2)")
                    .arg(m_sources.values()[index.row()]->name)
                    .arg(CID::CIDIntoQString(m_sources.keys()[index.row()]));
            return QVariant(sourceString);
        }
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
        if (parent.row() >= m_sources.count() || parent.row() < 0)
            return QModelIndex();
        if(m_sources.values()[parent.row()]->universes.count() > row)
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
        return m_sources.count();
    }

    // Universe count of source
    if(parent.isValid() && parent.internalPointer()==NULL)
    {
        if (parent.row() >= m_sources.count() || parent.row() < 0)
            return 0;
        return m_sources.values()[parent.row()]->universes.count();
    }

    return 0;
}
