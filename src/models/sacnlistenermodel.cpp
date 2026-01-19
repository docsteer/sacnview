#include "sacnlistenermodel.h"

#include "sacnlistener.h"

SACNListenerTableModel::SACNListenerTableModel(QObject * parent)
    : QAbstractTableModel(parent)
{
    // Get copy of listener list
    const QHash<quint16, sACNManager::wListener> listenerList = sACNManager::Instance().getListenerList();

    for (const auto & weakListener : listenerList)
    {

        sACNManager::tListener listener(weakListener);
        if (listener)
        {
            UniverseDetails details;
            details.weakListener = weakListener;
            details.universe = listener->universe();
            m_universeDetails.append(details);
        }
    }
}

SACNListenerTableModel::~SACNListenerTableModel() {}

int SACNListenerTableModel::rowCount(const QModelIndex & parent) const
{
    return static_cast<int>(m_universeDetails.size());
}

int SACNListenerTableModel::columnCount(const QModelIndex & parent) const
{
    return COL_COUNT;
}

QVariant SACNListenerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal) return QVariant();

    if (role != Qt::DisplayRole) return QVariant();

    switch (section)
    {
        case COL_UNIVERSE: return tr("Universe");
        case COL_MERGES_PER_SEC: return tr("Merges");
        case COL_BIND_UNICAST: return tr("Unicast");
        case COL_BIND_MULTICAST: return tr("Multicast");
        case COL_COUNT: break;
    }
    return QVariant();
}

QVariant SACNListenerTableModel::data(const QModelIndex & index, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();

    if (index.row() < 0 || index.row() >= m_universeDetails.size()) return QVariant();

    const UniverseDetails & details = m_universeDetails[index.row()];
    if (index.column() == COL_UNIVERSE) return details.universe;

    sACNManager::tListener listener(details.weakListener);
    if (listener.isNull()) return QVariant();

    switch (index.column())
    {
        default:
        case COL_COUNT: break;
        case COL_MERGES_PER_SEC: return listener->mergesPerSecond();
        case COL_BIND_UNICAST: return bindStatus(listener->getBindStatus().unicast);
        case COL_BIND_MULTICAST: return bindStatus(listener->getBindStatus().multicast);
    }

    return QVariant();
}

void SACNListenerTableModel::refresh()
{
    // Mark all merges dirty
    static const QVector<int> roles = {Qt::DisplayRole};
    emit dataChanged(index(0, COL_MERGES_PER_SEC), index(rowCount() - 1, COL_BIND_MULTICAST), roles);
}

QString SACNListenerTableModel::bindStatus(sACNRxSocket::eBindStatus bindStatus)
{
    switch (bindStatus)
    {
        case sACNRxSocket::BIND_UNKNOWN: return tr("Unknown"); break;
        case sACNRxSocket::BIND_OK: return tr("OK"); break;
        case sACNRxSocket::BIND_FAILED: return tr("Failed"); break;
    }
    return QString();
}
