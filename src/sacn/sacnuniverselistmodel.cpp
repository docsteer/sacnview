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

#include "sacnuniverselistmodel.h"
#include <QDebug>
#include <QNetworkInterface>
#include "streamingacn.h"
#include "streamcommon.h"
#include "ipaddr.h"
#include "preferences.h"
#include "sacnsocket.h"
#include "consts.h"

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

sACNBasicSourceInfo::sACNBasicSourceInfo(sACNUniverseInfo *p)
{
    parent = p;
}


sACNUniverseListModel::sACNUniverseListModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_start = 1;

    m_socket = new sACNRxSocket(this);
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

    m_socket->bindMulticast(1);

    setStartUniverse(1);

    m_checkTimeoutTimer = new QTimer(this);
    connect(m_checkTimeoutTimer, SIGNAL(timeout()), this, SLOT(checkTimeouts()));
    m_checkTimeoutTimer->start(5000);
}

void sACNUniverseListModel::setStartUniverse(int start)
{
    // Limit max value
    static const int startMax = (MAX_SACN_UNIVERSE - NUM_UNIVERSES_LISTED) + 1;
    if (start > startMax) start = startMax;

    beginResetModel();

    qDeleteAll(m_universes);
    m_universes.clear();

    for(int universe=m_start; universe<m_start+NUM_UNIVERSES_LISTED; universe++)
    {
        CIPAddr addr;
        GetUniverseAddress(universe, addr);
        QNetworkInterface iface = Preferences::getInstance()->networkInterface();
        m_socket->leaveMulticastGroup(QHostAddress(addr.GetV4Address()), iface);
    }

    m_start = start;

    for(int universe=m_start; universe<m_start+NUM_UNIVERSES_LISTED; universe++)
    {
        CIPAddr addr;
        GetUniverseAddress(universe, addr);
        QNetworkInterface iface = Preferences::getInstance()->networkInterface();
        m_socket->joinMulticastGroup(QHostAddress(addr.GetV4Address()), iface);

        m_universes << new sACNUniverseInfo(universe);
    }

    endResetModel();
}


int sACNUniverseListModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid() && parent.internalPointer()==NULL)
    {
        return m_universes[parent.row()]->sources.count();

    }
    if(!parent.isValid())
    {
        return m_universes.count();
    }

    return 0;
}

int sACNUniverseListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant sACNUniverseListModel::data(const QModelIndex &index, int role) const
{
    if(role==Qt::DisplayRole)
    {
        if(index.parent().isValid())
        {
            sACNBasicSourceInfo *info = (sACNBasicSourceInfo *) index.internalPointer();
            return tr("%1 (%2)").arg(info->name).arg(info->address.toString());
        }
        else
            return QVariant(tr("Universe %1").arg(index.row() + m_start));
    }
    return QVariant();
}

QModelIndex sACNUniverseListModel::index(int row, int column, const QModelIndex &parent) const
{
    if(!hasIndex(row, column, parent))
        return QModelIndex();
    if(!parent.isValid()) // This is a root item
        return createIndex(row, column);
    if(parent.isValid() && !parent.parent().isValid())
    {
        if(m_universes[parent.row()]->sources.count() >= row)
        {
            return createIndex(row, column, m_universes[parent.row()]->sources.at(row));
        }
    }

    return QModelIndex();
}

QModelIndex sACNUniverseListModel::parent(const QModelIndex &index) const
{
    if(!index.isValid())
        return QModelIndex();

    sACNBasicSourceInfo *i = static_cast<sACNBasicSourceInfo *>(index.internalPointer());
    if(i)
    {
        int parentRow = i->parent->universe - m_start;
        return createIndex(parentRow, 0);
    }

    return QModelIndex();
}

void sACNUniverseListModel::readPendingDatagrams()
{
    while(m_socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_socket->readDatagram(datagram.data(), datagram.size(),
                                &sender, &senderPort);

        // Process the data
        CID source_cid;
        uint1 start_code;
        uint1 sequence;
        uint2 universe;
        uint2 slot_count;
        uint1* pdata;
        char source_name [SOURCE_NAME_SIZE];
        uint1 priority;
        //These only apply to the ratified version of the spec, so we will hardwire
        //them to be 0 just in case they never get set.
        uint2 reserved = 0;
        uint1 options = 0;
        bool preview = false;
        uint1 *pbuf = (uint1*)datagram.data();

        if(!ValidateStreamHeader(pbuf, datagram.length(), source_cid, source_name, priority,
                start_code, reserved, sequence, options, universe, slot_count, pdata))
        {
            // Recieved a packet but not valid. Log and discard
            qDebug() << "Invalid Packet";
            continue;
        }

        // Listen to preview?
        preview = (PREVIEW_DATA_OPTION == (options & PREVIEW_DATA_OPTION));
        if ((preview) && !Preferences::getInstance()->GetBlindVisualizer())
        {
            qDebug() << "Ignore preview";
            return;
        }

        sACNBasicSourceInfo *info = 0;
        int univIndex = universe - m_start;

        if(!m_universes[univIndex]->sourcesByCid.contains(source_cid))
        {
            info = new sACNBasicSourceInfo(m_universes[univIndex]);
            info->cid = source_cid;
        }
        else
        {
            info = m_universes[univIndex]->sourcesByCid.value(source_cid);
            info->timeout.restart();
        }

        info->address = sender;
        info->name = source_name;

        if(!m_universes[univIndex]->sourcesByCid.contains(source_cid))
        {
            // We are adding the source for this universe
            QModelIndex parent = index(m_start - universe, 0);
            int firstRow = m_universes[univIndex]->sources.count()+1;
            int lastRow = firstRow;
            beginInsertRows(parent, firstRow, lastRow);
            m_universes[univIndex]->sources << info;
            m_universes[univIndex]->sourcesByCid[source_cid] = info;
            endInsertRows();
        }

    }
}

void sACNUniverseListModel::checkTimeouts()
{
    foreach(sACNUniverseInfo *info, m_universes)
    {
        for(int row=0; row<info->sources.count(); row++)
        {
            sACNBasicSourceInfo *source = info->sources[row];
            if(source->timeout.elapsed() > 5000)
            {
                beginRemoveRows( createIndex(info->universe - m_start, 0),
                            row, row);
                info->sources.removeAll(source);
                info->sourcesByCid.remove(source->cid);
                delete source;
                endRemoveRows();
            }
        }
    }
}

int sACNUniverseListModel::indexToUniverse(const QModelIndex &index)
{
    if(!index.isValid())
        return 0;

    if(index.internalPointer()==NULL)
    {
        // root item
        return m_start + index.row();
    }

    sACNBasicSourceInfo *i = static_cast<sACNBasicSourceInfo *>(index.internalPointer());
    if(i)
    {
        return i->parent->universe;
    }

    return 0;
}
