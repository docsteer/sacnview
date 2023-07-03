// Copyright 2023 Richard Thompson
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

#include "sacnsourcetablemodel.h"

#include "preferences.h"

#include "sacn/sacnlistener.h"

SACNSourceTableModel::SACNSourceTableModel(QObject* parent)
  : QAbstractTableModel(parent)
{
}

SACNSourceTableModel::~SACNSourceTableModel()
{
}

bool SACNSourceTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (role != Qt::EditRole)
    return false;

  // Sequence and Jump counters can be reset
  switch (index.column())
  {
  default: break;
  case COL_SEQ_ERR: // TODO
  case COL_JUMPS: // TODO
    break;
  }
  return false;
}

QVariant SACNSourceTableModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (index.row() >= rowCount())
    return QVariant();

  const RowData& rowData = m_rows[index.row()];

  switch (role)
  {
  case Qt::DisplayRole: return getDisplayData(rowData, index.column());

  case Qt::BackgroundRole: return getBackgroundData(rowData, index.column());

  case Qt::TextAlignmentRole:
    switch (index.column())
    {
      // Text
    case COL_NAME:
    case COL_CID:
    case COL_TIME_SUMMARY:
      return static_cast<Qt::Alignment::Int>(Qt::AlignVCenter | Qt::AlignLeft);
      // Numeric
    case COL_UNIVERSE:
    case COL_PRIO:
    case COL_SYNC:
    case COL_FPS:
    case COL_SEQ_ERR:
    case COL_JUMPS:
    case COL_SLOTS:
      return static_cast<Qt::Alignment::Int>(Qt::AlignVCenter | Qt::AlignRight);
      // Flag
    case COL_ONLINE:
    case COL_IP:
    case COL_PREVIEW:
    case COL_VER:
    case COL_DD:
    case COL_PATHWAY_SECURE:
      return static_cast<Qt::Alignment::Int>(Qt::AlignCenter);
    }
  }
  return QVariant();
}


QVariant SACNSourceTableModel::getDisplayData(const RowData& rowData, int column) const
{
  switch (column)
  {
  default: break;
  case COL_NAME: return rowData.name;
  case COL_ONLINE:
  {
    switch (rowData.online)
    {
    case SourceState::Offline: return tr("Offline");
    case SourceState::NoDmx: return tr("No DMX");
    case SourceState::Unstable: return tr("Online (Unstable)");
    case SourceState::Stable: return tr("Online");
    }
    return QStringLiteral("??");
  }
  case COL_CID: return rowData.cid;
  case COL_UNIVERSE: return rowData.universe;
  case COL_PRIO: return rowData.per_address ? QStringLiteral("(*) ") + QString::number(rowData.priority) : QString::number(rowData.priority);
  case COL_SYNC:
    if (rowData.protocol_version == sACNProtocolDraft)
      return tr("N/A");
    else
      return (rowData.sync_universe != 0
        ? QString(tr("Yes (%1)")).arg(rowData.sync_universe)
        : tr("No"));
  case COL_PREVIEW: return (rowData.preview ? tr("Yes") : tr("No"));
  case COL_IP: return rowData.ip.toString();
  case COL_FPS: return QStringLiteral("%1Hz").arg(QString::number(rowData.fps, 'f', 2));
  case COL_TIME_SUMMARY: return getTimingSummary(rowData);
  case COL_SEQ_ERR: return rowData.seq_err;
  case COL_JUMPS: return rowData.jumps;
  case COL_VER: return GetProtocolVersionString(rowData.protocol_version);
  case COL_DD: return (rowData.per_address ?
    (Preferences::Instance().GetETCDD() ? tr("Yes") : tr("Ignored"))
    : tr("No"));
  case COL_SLOTS: return rowData.slot_count;
  case COL_PATHWAY_SECURE:
    switch (rowData.security)
    {
    case SourceSecure::None: return (Preferences::Instance().GetPathwaySecureRxDataOnly() ? tr("No") : tr("N/A"));
    case SourceSecure::BadDigest: return tr("Bad Message Digest");
    case SourceSecure::BadSequence: return tr("Bad Sequence");
    case SourceSecure::BadPassword: return tr("Bad Password");
    case SourceSecure::Yes: return tr("Yes");
    }
  }
  return QVariant();
}

QVariant SACNSourceTableModel::getBackgroundData(const RowData& rowData, int column) const
{
  switch (column)
  {
  default: break;
  case COL_NAME: return Preferences::Instance().colorForCID(rowData.cid);
  case COL_ONLINE: switch (rowData.online)
  {
  default:
  case SourceState::Offline: return Preferences::Instance().colorForStatus(Preferences::Status::Bad);
  case SourceState::NoDmx: return Preferences::Instance().colorForStatus(Preferences::Status::Warning);
  case SourceState::Unstable: return Preferences::Instance().colorForStatus(Preferences::Status::Warning);
  case SourceState::Stable: return Preferences::Instance().colorForStatus(Preferences::Status::Good);
  }
  case COL_PATHWAY_SECURE:
    switch (rowData.security)
    {
    case SourceSecure::None: return (Preferences::Instance().GetPathwaySecureRxDataOnly() ? Preferences::Instance().colorForStatus(Preferences::Status::Bad) : QColor(Qt::transparent));
    default:
    case SourceSecure::BadDigest: return Preferences::Instance().colorForStatus(Preferences::Status::Bad);
    case SourceSecure::BadSequence: return Preferences::Instance().colorForStatus(Preferences::Status::Warning);
    case SourceSecure::BadPassword: return Preferences::Instance().colorForStatus(Preferences::Status::Warning);
    case SourceSecure::Yes: return Preferences::Instance().colorForStatus(Preferences::Status::Good);
    }
  case COL_CID:
  case COL_UNIVERSE:
  case COL_PRIO:
  case COL_SYNC:
  case COL_PREVIEW:
  case COL_IP:
  case COL_FPS:
  case COL_SEQ_ERR:
  case COL_JUMPS:
  case COL_VER:
  case COL_DD:
  case COL_SLOTS:
    break;
  }

  return QVariant();
}

QVariant SACNSourceTableModel::getTimingSummary(const RowData& rowData) const
{
  const FpsCounter::Histogram& histogram = rowData.histogram;
  if (histogram.empty())
    return tr("N/A");

  // Min - max
  QString result = QStringLiteral("%1-%2ms").arg(std::chrono::milliseconds(histogram.begin()->first).count()).arg(std::chrono::milliseconds(histogram.rbegin()->first).count());
  // And count of interesting ranges
  size_t shortCount = 0;
  size_t longCount = 0;
  size_t staticCount = 0;
  for (const auto& item : histogram)
  {
    if (item.first < m_shortInterval)
      shortCount += item.second;
    else if (item.first > m_staticInterval)
      staticCount += item.second;
    else if (item.first > m_longInterval)
      longCount += item.second;
  }

  result.append(QStringLiteral(" (%1 %2 %3)").arg(shortCount).arg(longCount).arg(staticCount));
  return result;
}


QVariant SACNSourceTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal)
  {
    switch (role)
    {
    case Qt::DisplayRole:
      switch (section)
      {
      default: break;
      case COL_NAME: return tr("Name");
      case COL_ONLINE: return tr("Online");
      case COL_CID: return tr("CID");
      case COL_UNIVERSE: return tr("Universe");
      case COL_PRIO: return tr("Priority");
      case COL_SYNC: return tr("Sync");
      case COL_PREVIEW: return tr("Preview");
      case COL_IP: return tr("IP Address");
      case COL_FPS: return tr("FPS");
      case COL_TIME_SUMMARY: return tr("Times (<%1 >%2 Static)").arg(std::chrono::milliseconds(m_shortInterval).count()).arg(std::chrono::milliseconds(m_longInterval).count());
      case COL_SEQ_ERR: return tr("SeqErr");
      case COL_JUMPS: return tr("Jumps");
      case COL_VER: return tr("Ver");
      case COL_DD: return tr("Per-Address");
      case COL_SLOTS: return tr("Slots");
      case COL_PATHWAY_SECURE: return tr("Secure");
      } break;
    case Qt::ToolTipRole:
      switch (section)
      {
      default: break;
      case COL_NAME: return tr("The human readable name the source has been given");
      case COL_ONLINE: return tr("Online status of the source");
      case COL_CID: return tr("The Component IDentifier of the source, a Universally Unique Identifier");
      case COL_UNIVERSE: return tr("sACN Universe number (%1-%1)").arg(MIN_SACN_UNIVERSE).arg(MAX_SACN_UNIVERSE);
      case COL_PRIO: return tr("Source priority %1 (ignore) to %2 (most important). Default %1").arg(MIN_SACN_PRIORITY).arg(MAX_SACN_PRIORITY).arg(DEFAULT_SACN_PRIORITY);
      case COL_SYNC: return tr("Does the source support Universe Synchronization?");
      case COL_PREVIEW: return tr("Indicates that the data in this packet is intended for use in visualization or media server preview applications and shall not be used to generate live output.");
      case COL_IP: return tr("The IP address of the source");
      case COL_FPS: return tr("Frames per Second, aka refresh rate of the DMX source");
      case COL_SEQ_ERR: return tr("Number of packets which have received out of order");
      case COL_JUMPS: return tr("Number of packets which have been missed");
      case COL_VER: return tr("Protocol version");
      case COL_DD: return tr("If the source supports Electronic Theatre Controls Per-Address extension, is the source transmitting per address priorities?");
      case COL_SLOTS: return tr("Number of DMX Data slots the source is transmitting");
      case COL_PATHWAY_SECURE: return tr("If the source supports Pathways Secure DMX extension, is the password correct and the packet secure?");
      }
      break;
    }
  }
  return QVariant();
}

void SACNSourceTableModel::addListener(const sACNManager::tListener& listener)
{
  if (!listener)
    return;

  // Start listening for new sources
  connect(listener.data(), &sACNListener::sourceFound, this, &SACNSourceTableModel::sourceOnline);
  connect(listener.data(), &sACNListener::sourceLost, this, &SACNSourceTableModel::sourceChanged);
  connect(listener.data(), &sACNListener::sourceChanged, this, &SACNSourceTableModel::sourceChanged);

  // Now add pre-existing sources
  for (int i = 0; i < listener->sourceCount(); i++)
  {
    sourceOnline(listener->source(i));
  }
  m_listeners.push_back(listener);
}

void SACNSourceTableModel::pause()
{
  // Stop listening (queued events will still happen)
  for (size_t i = 0; i < m_listeners.size(); ++i)
  {
    sACNManager::tListener listener(m_listeners[i]);
    if (listener)
      disconnect(listener.data(), nullptr, this, nullptr);
  }
}

void SACNSourceTableModel::restart()
{
  // Restart listening on all still extant universes
  for (auto it = m_listeners.begin(); it != m_listeners.end(); /**/)
  {
    sACNManager::tListener listener(*it);
    if (listener)
    {
      connect(listener.data(), &sACNListener::sourceFound, this, &SACNSourceTableModel::sourceOnline);
      connect(listener.data(), &sACNListener::sourceLost, this, &SACNSourceTableModel::sourceChanged);
      connect(listener.data(), &sACNListener::sourceChanged, this, &SACNSourceTableModel::sourceChanged);
      ++it;
    }
    else
    {
      // Remove nulled weak pointers
      it = m_listeners.erase(it);
    }
  }
}

void SACNSourceTableModel::clear()
{
  pause();
  m_listeners.clear();

  // Clear the model
  beginResetModel();
  m_rows.clear();
  m_sourceToTableRow.clear();
  endResetModel();
}

void SACNSourceTableModel::resetSequenceCounters()
{
  for (auto it = m_sourceToTableRow.begin(); it != m_sourceToTableRow.end(); ++it)
  {
    it.key()->resetSeqErr();
  }
  for (auto& row : m_rows)
  {
    row.seq_err = 0;
  }
  emit dataChanged(index(0, COL_SEQ_ERR), index(rowCount() - 1, COL_SEQ_ERR));
}

void SACNSourceTableModel::resetJumpsCounters()
{
  for (auto it = m_sourceToTableRow.begin(); it != m_sourceToTableRow.end(); ++it)
  {
    it.key()->resetJumps();
  }
  for (auto& row : m_rows)
  {
    row.jumps = 0;
  }
  emit dataChanged(index(0, COL_JUMPS), index(rowCount() - 1, COL_JUMPS));
}

void SACNSourceTableModel::resetCounters()
{
  for (auto it = m_sourceToTableRow.begin(); it != m_sourceToTableRow.end(); ++it)
  {
    it.key()->resetSeqErr();
    it.key()->resetJumps();
  }
  for (auto& row : m_rows)
  {
    row.seq_err = 0;
    row.jumps = 0;
  }
  emit dataChanged(index(0, COL_SEQ_ERR), index(rowCount() - 1, COL_JUMPS));
}

void SACNSourceTableModel::sourceChanged(sACNSource* source)
{
  if (!source)
    return;

  const int row_num = m_sourceToTableRow.value(source, -1);
  if (row_num < 0)
  {
    sourceOnline(source);
    return;
  }

  // Update and signal
  m_rows[row_num].Update(source);
  emit dataChanged(index(row_num, 0), index(row_num, COL_END - 1));
}

void SACNSourceTableModel::sourceOnline(sACNSource* source)
{
  if (!source)
    return;

  const int row = m_rows.size();
  beginInsertRows(QModelIndex(), row, row);
  m_sourceToTableRow[source] = row;
  m_rows.emplace_back(source);
  endInsertRows();
}

SACNSourceTableModel::RowData::RowData(const sACNSource* source)
{
  Update(source);
}

void SACNSourceTableModel::RowData::Update(const sACNSource* source)
{
  if (!source)
    return;

  name = source->name;
  cid = source->src_cid;
  universe = source->universe;
  protocol_version = source->protocol_version;
  ip = source->ip;
  fps = source->fpscounter.FPS();
  seq_err = source->seqErr;
  jumps = source->jumps;
  if (source->src_valid)
  {
    if (source->doing_dmx)
      online = source->src_stable ? online = SourceState::Stable : online = SourceState::Unstable;
    else
      online = SourceState::NoDmx;
  }
  else
  {
    online = SourceState::Offline;
  }

  if (source->protocol_version == sACNProtocolPathwaySecure) {
    if (source->pathway_secure.isSecure()) {
      security = SourceSecure::Yes;
    }
    else {
      if (!source->pathway_secure.passwordOk) {
        security = SourceSecure::BadPassword;
      }
      else if (!source->pathway_secure.sequenceOk) {
        security = SourceSecure::BadSequence;
      }
      else if (!source->pathway_secure.digestOk) {
        security = SourceSecure::BadDigest;
      }
    }
  }

  sync_universe = source->synchronization;
  slot_count = source->slot_count;
  priority = source->priority;
  preview = source->isPreview;
  per_address = source->doing_per_channel;
  histogram = source->fpscounter.GetHistogram();
}
