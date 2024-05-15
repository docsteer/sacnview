// Copyright 2023 Electronic Theatre Controls, Inc. or its affiliates
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

#include "glscopewidget.h"

#include <QPainter>
#include <QScreen>
#include <QOpenGLShaderProgram>
#include <QMetaEnum>
#include <QMouseEvent>

static constexpr qreal AXIS_LABEL_WIDTH = 45.0;
static constexpr qreal AXIS_LABEL_HEIGHT = 20.0;
static constexpr qreal TOP_GAP = 10.0;
static constexpr qreal RIGHT_GAP = 15.0;
static constexpr qreal AXIS_TO_WINDOW_GAP = 5.0;
static constexpr qreal AXIS_TICK_SIZE = 10.0;

static const QString CaptureOptionsTitle = QStringLiteral("Capture Options");
static const QString RowTitleLabel = QStringLiteral("Label");
static const QString RowTitleColor = QStringLiteral("Color");
static const QString ColumnTitleWallclockTime = QStringLiteral("Wallclock");
static const QString ColumnTitleTimestamp = QStringLiteral("Time (s)");

static const QString ShortTimeFormatString = QStringLiteral("hh:mm:ss");
static const QString TimeFormatString = QStringLiteral("hh:mm:ss.zzz");
static const QString DateTimeFormatString = QStringLiteral("yyyy-MM-dd ") + TimeFormatString;

static constexpr qreal kMaxDmx16 = 65535;
static constexpr qreal kMaxDmx8 = 255;

template<typename T>
T roundCeilMultiple(T value, T multiple)
{
  if (multiple == 0) return value;
  return static_cast<T>(std::ceil(static_cast<qreal>(value) / static_cast<qreal>(multiple)) * static_cast<qreal>(multiple));
}

bool ScopeTrace::extractUniverseAddress(QStringView address_string, uint16_t& universe, uint16_t& address_hi, uint16_t& address_lo)
{
  if (address_string.front() != QLatin1Char('U'))
    return false;

  // Extract the universe
  const qsizetype univ_str_end = address_string.indexOf(QLatin1Char('.'));
  bool ok = false;
  if (univ_str_end > 0)
  {
    const QStringView univ_str = address_string.mid(1, univ_str_end - 1);
    universe = univ_str.toUInt(&ok);
    if (!ok || universe == 0 || universe > MAX_SACN_UNIVERSE)
      return false;

    return extractAddress(address_string.mid(univ_str_end + 1), address_hi, address_lo);
  }

  return false;
}

bool ScopeTrace::extractAddress(QStringView address_string, uint16_t& address_hi, uint16_t& address_lo)
{
  // Extract address_hi
  const qsizetype addr_str_end = address_string.indexOf(QLatin1Char('/'));
  bool ok = false;
  if (addr_str_end > 0)
  {
    const QStringView slot_hi_str = address_string.left(addr_str_end);
    address_hi = slot_hi_str.toUInt(&ok);
    if (!ok)
      return false;

    // Extract slot_lo
    const QStringView slot_lo_str = address_string.mid(addr_str_end + 1);
    address_lo = slot_lo_str.toUInt(&ok);
  }
  else
  {
    const QStringView slot_hi_str = address_string.left(addr_str_end);
    address_hi = slot_hi_str.toUInt(&ok);
    address_lo = 0;
    if (!ok)
      return false;
  }

  return ok;
}

QString ScopeTrace::universeAddressString(uint16_t universe, uint16_t address_hi, uint16_t address_lo)
{
  return QStringLiteral("U") + QString::number(universe) + QLatin1Char('.') + addressString(address_hi, address_lo);
}

QString ScopeTrace::addressString(uint16_t address_hi, uint16_t address_lo)
{
  if (address_lo > 0 && address_lo <= MAX_DMX_ADDRESS)
    return QString::number(address_hi) + QLatin1Char('/') + QString::number(address_lo);
  return QString::number(address_hi);
}

QString ScopeTrace::universeAddressString() const
{
  return universeAddressString(universe(), addressHi(), addressLo());
}

QString ScopeTrace::addressString() const
{
  return addressString(addressHi(), addressLo());
}

bool ScopeTrace::setUniverse(uint16_t new_universe, bool clear_values)
{
  // Invalid or unchanged
  if (new_universe == m_universe || new_universe < 0 || new_universe > MAX_SACN_UNIVERSE)
    return false;

  m_universe = new_universe;

  if (clear_values)
    clear();

  return true;
}

bool ScopeTrace::setAddress(uint16_t address_hi, uint16_t address_lo, bool clear_values)
{
  // Validate
  if (address_hi < 1 || address_hi > MAX_DMX_ADDRESS || address_lo > MAX_DMX_ADDRESS)
    return false;

  if (addressHi() == address_hi && addressLo() == address_lo)
    return false; // No change

  m_slot_hi = address_hi - 1;
  m_slot_lo = address_lo == 0 ? MAX_DMX_ADDRESS : address_lo - 1;

  if (clear_values)
    clear();

  return true;
}

bool ScopeTrace::setAddress(QStringView addressString, bool clear_values)
{
  uint16_t address_hi, address_lo;
  if (extractAddress(addressString, address_hi, address_lo))
    return setAddress(address_hi, address_lo, clear_values);

  return false;
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
inline bool fillValue(T& value, uint16_t slot_hi, uint16_t slot_lo, const sACNMergedSourceList& level_array, const sACNSource* source)
{
  // Assumes slot numbers are valid
  if (slot_lo < level_array.size())
  {
    // 16 bit
    // Do nothing if not the winning source
    if (level_array[slot_hi].winningSource != source && level_array[slot_lo].winningSource != source)
      return false;

    // Do nothing if no level yet
    if (level_array[slot_hi].level < 0 || level_array[slot_lo].level < 0)
      return false;

    value = static_cast<uint16_t>(level_array[slot_hi].level) << 8 | static_cast<uint16_t>(level_array[slot_lo].level);
    return true;
  }
  // 8 bit
  // Do nothing if not the winning source
  if (level_array[slot_hi].winningSource != source)
    return false;

  // Do nothing if no level
  if (level_array[slot_hi].level < 0)
    return false;

  value = static_cast<T>(level_array[slot_hi].level);
  return true;
}

void ScopeTrace::addPoint(float timestamp, const sACNMergedSourceList& level_array, const sACNSource* source, bool storeAllPoints)
{
  float value;
  if (fillValue(value, m_slot_hi, m_slot_lo, level_array, source))
  {
    QMutexLocker lock(&m_mutex);
    if (storeAllPoints)
    {
      m_trace.emplace_back(timestamp, value);
      return;
    }
    // If level did not change in the last two, only update timestamp
    const size_t trace_size = m_trace.size();
    if (trace_size > 2 && m_trace[trace_size - 1].y() == value && m_trace[trace_size - 2].y() == value)
    {
      m_trace.back().setX(timestamp);
    }
    else
    {
      m_trace.emplace_back(timestamp, value);
    }
  }
}

void ScopeTrace::setFirstPoint(float timestamp, const sACNMergedSourceList& level_array, const sACNSource* source)
{
  float value;
  if (fillValue(value, m_slot_hi, m_slot_lo, level_array, source))
  {
    QMutexLocker lock(&m_mutex);
    if (m_trace.empty())
      m_trace.emplace_back(timestamp, value);
    else
      m_trace[0] = { timestamp,value };
  }
}

void ScopeTrace::applyOffset(float offset)
{
  QMutexLocker lock(&m_mutex);
  for (auto& point : m_trace)
    point.setX(point.x() - offset);
}

ScopeModel::AddResult ScopeModel::addTrace(const QColor& color, uint16_t universe, uint16_t address_hi, uint16_t address_lo)
{
  // Verify validity
  if (!color.isValid())
    return AddResult::Invalid;

  if (universe < MIN_SACN_UNIVERSE)
    return AddResult::Invalid;

  if (address_hi == 0)
    return AddResult::Invalid;

  if (universe > MAX_SACN_UNIVERSE)
    return AddResult::Invalid;

  if (address_hi > MAX_DMX_ADDRESS)
    return AddResult::Invalid;

  if (address_lo > MAX_DMX_ADDRESS)
    address_lo = 0;
  else if (address_lo > 0)
  {
    // 16bit, adjust vertical scale
    setMaxValue(kMaxDmx16);
  }

  auto univ_it = m_traceLookup.find(universe);
  if (univ_it == m_traceLookup.end())
  {
    // Set the first one to be the trigger
    if (m_traceTable.empty())
    {
      m_trigger.universe = universe;
      m_trigger.address_hi = address_hi;
      m_trigger.address_lo = address_lo;
    }

    beginInsertRows(QModelIndex(), m_traceTable.size(), m_traceTable.size());
    ScopeTrace* trace = new ScopeTrace(color, universe, address_hi, address_lo, m_reservation);
    m_traceTable.push_back(trace);
    m_traceLookup.emplace(universe, std::vector<ScopeTrace*>(1, trace));
    endInsertRows();

    // Maybe start listening
    if (isRunning())
    {
      addListener(universe);
    }

    return AddResult::Added;
  }

  // If we have already got this trace, skip
  auto& univs_item = univ_it->second;
  for (auto& item : univs_item)
  {
    if (item->addressHi() == address_hi && item->addressLo() == address_lo)
    {
      return AddResult::Exists;
    }
  }

  // Add this new trace to the existing universe
  beginInsertRows(QModelIndex(), m_traceTable.size(), m_traceTable.size());
  ScopeTrace* trace = new ScopeTrace(color, universe, address_hi, address_lo, m_reservation);
  m_traceTable.push_back(trace);
  univs_item.push_back(trace);
  endInsertRows();
  return AddResult::Added;
}

ScopeModel::AddResult ScopeModel::addTrace(const QString& label, const QColor& color, uint16_t universe, uint16_t address_hi, uint16_t address_lo)
{
  const AddResult result = addTrace(color, universe, address_hi, address_lo);
  if (result == AddResult::Added && !label.isEmpty())
  {
    ScopeTrace* trace = findTrace(universe, address_hi, address_lo);
    if (trace)
      trace->setLabel(label);
  }
  return result;
}

// Removes all traces that match this
void ScopeModel::removeTrace(uint16_t universe, uint16_t address_hi, uint16_t address_lo)
{
  auto univ_it = m_traceLookup.find(universe);
  if (univ_it == m_traceLookup.end())
    return;

  bool found = false;
  auto& univ_item = univ_it->second;
  for (auto it = univ_item.begin(); it != univ_item.end(); /**/)
  {
    if ((*it)->addressHi() == address_hi && (*it)->addressLo() == address_lo)
    {
      found = true;
      it = univ_item.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // Find the index in the tracetable and delete the object
  if (found)
  {
    for (auto it = m_traceTable.begin(); it != m_traceTable.end(); /**/)
    {
      ScopeTrace* trace = (*it);
      if (trace->universe() == universe && trace->addressHi() == address_hi && trace->addressLo() == address_lo)
      {
        const int row = it - m_traceTable.begin();
        beginRemoveRows(QModelIndex(), row, row);
        it = m_traceTable.erase(it);
        delete trace;
        endRemoveRows();
      }
      else
      {
        ++it;
      }
    }
  }

  // If last trace on a universe, stop listening
  if (univ_item.empty())
  {
    for (auto it = m_listeners.begin(); it != m_listeners.end(); /**/)
    {
      if ((*it)->universe() == universe)
      {
        // Disconnect signal
        (*it)->removeDirectCallback(this);
        it = m_listeners.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
}

void ScopeModel::removeTraces(const QModelIndexList& indexes)
{
  // Create list of items to remove
  std::vector<size_t> traceRows;
  for (const QModelIndex& idx : indexes)
  {
    if (idx.isValid() && idx.row() < rowCount())
    {
      traceRows.push_back(idx.row());
    }
  }

  // Start at the end
  std::sort(traceRows.begin(), traceRows.end(), std::greater<size_t>());

  size_t prevRow = std::numeric_limits<size_t>::max();

  for (size_t row : traceRows)
  {
    if (prevRow == row)
      continue; // This was a duplicate

    beginRemoveRows(QModelIndex(), row, row);

    ScopeTrace* trace = m_traceTable[row];
    removeFromLookup(trace, trace->universe());

    m_traceTable.erase(m_traceTable.begin() + row);
    delete trace;
    endRemoveRows();

    prevRow = row;
  }
}

ScopeTrace* ScopeModel::findTrace(uint16_t universe, uint16_t address_hi, uint16_t address_lo)
{
  const auto univ_it = m_traceLookup.find(universe);
  if (univ_it != m_traceLookup.end())
  {
    const auto& univ_item = univ_it->second;
    for (const auto& item : univ_item)
    {
      if (item->addressHi() == address_hi && item->addressLo() == address_lo)
        return item;
    }
  }

  // Not found
  return nullptr;
}

QModelIndex ScopeModel::findFirstTraceIndex(uint16_t universe, uint16_t address_hi, uint16_t address_lo, int column) const
{
  for (size_t row = 0; row < m_traceTable.size(); ++row)
  {
    const ScopeTrace* trace = m_traceTable[row];
    if (trace && trace->universe() == universe && trace->addressHi() == address_hi && trace->addressLo() == address_lo)
    {
      return index(row, column);
    }
  }
  return QModelIndex();
}

ScopeModel::ScopeModel(QObject* parent)
  : QAbstractTableModel(parent)
{
  private_removeAllTraces();
  connect(this, &ScopeModel::queueStop, this, &ScopeModel::stop, Qt::QueuedConnection);
  connect(this, &ScopeModel::queueTriggered, this, &ScopeModel::triggered, Qt::QueuedConnection);
  connect(this, &ScopeModel::queueTriggered, this, &ScopeModel::onQueueTriggered, Qt::QueuedConnection);
}

ScopeModel::~ScopeModel()
{
  private_removeAllTraces();
}

QVariant ScopeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
  {
    switch (section)
    {
    case COL_UNIVERSE: return tr("Universe");
    case COL_ADDRESS: return tr("Address");
    case COL_COLOUR: return tr("Colour");
    case COL_TRIGGER: return tr("Trigger");
    case COL_LABEL: return tr("Label");
    }
  }
  return QVariant();
}

int ScopeModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return m_traceTable.size();
}

Qt::ItemFlags ScopeModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  switch (index.column())
  {
  default: return Qt::ItemFlags();
  case  COL_UNIVERSE: return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
  case  COL_ADDRESS: return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  case  COL_COLOUR: return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  case  COL_TRIGGER: return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
  case  COL_LABEL: return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
}

QVariant ScopeModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (index.column() < COL_COUNT && index.row() < rowCount())
  {
    const ScopeTrace* trace = m_traceTable.at(index.row());
    if (trace)
    {
      switch (index.column())
      {
      default: break;
      case COL_UNIVERSE:
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == DataSortRole) return trace->universe();
        if (role == Qt::CheckStateRole) return trace->enabled() ? Qt::Checked : Qt::Unchecked;
        if (role == Qt::ToolTipRole) return tr("sACN Universe");
        break;
      case COL_ADDRESS:
        if (role == DataSortRole) return uint32_t(trace->addressHi()) << 16 | trace->addressLo();
        if (role == Qt::DisplayRole || role == Qt::EditRole) return trace->addressString();
        if (role == Qt::ToolTipRole) return tr("DMX Address. MSB/LSB for 16bit");
        break;
      case COL_COLOUR:
        if (role == Qt::BackgroundRole || role == Qt::DisplayRole || role == Qt::EditRole) return trace->color();
        if (role == DataSortRole) return static_cast<uint32_t>(trace->color().rgba());
        if (role == Qt::ToolTipRole) return tr("Trace color (#RRGGBB)");
        break;
      case COL_TRIGGER:
        if (role == Qt::CheckStateRole)
          return m_trigger.isTriggerTrace(*trace) ? Qt::Checked : Qt::Unchecked;
        if (role == DataSortRole) return m_trigger.isTriggerTrace(*trace) ? 0 : 1;
        if (role == Qt::ToolTipRole) return tr("Trigger on this trace");
        break;
      case COL_LABEL:
        if (role == Qt::DisplayRole || role == Qt::EditRole || role == DataSortRole) return trace->label();
        break;
      }
    }
  }

  return QVariant();
}

bool ScopeModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  if (!idx.isValid())
    return false;

  if (idx.column() >= COL_COUNT || idx.row() >= rowCount())
    return false;

  ScopeTrace* trace = m_traceTable.at(idx.row());
  if (!trace)
    return false;

  switch (idx.column())
  {
  default: break;

  case COL_UNIVERSE:
    if (role == Qt::CheckStateRole)
    {
      trace->setEnabled(value.toBool());
      emit dataChanged(idx, idx, { Qt::CheckStateRole });
      emit traceVisibilityChanged();
      return true;
    }
    if (role == Qt::EditRole)
    {
      // Maybe update the trigger
      const bool isTrigger = m_trigger.isTriggerTrace(*trace);
      if (moveTrace(trace, value.toUInt()))
      {
        if (isTrigger)
          m_trigger.setTrigger(*trace);
        emit dataChanged(idx, idx, { Qt::DisplayRole, Qt::EditRole, DataSortRole });
        return true;
      }
    }
    break;

  case COL_ADDRESS:
    if (role == Qt::EditRole)
    {
      // If 16bit changed, recheck the vertical scale
      const bool was16bit = trace->isSixteenBit();
      // Maybe update the trigger
      const bool isTrigger = m_trigger.isTriggerTrace(*trace);
      if (trace->setAddress(value.toString(), true))
      {
        if (was16bit != trace->isSixteenBit())
        {
          if (trace->isSixteenBit())
            setMaxValue(kMaxDmx16);
          else
            updateMaxValue();
        }
        if (isTrigger)
          m_trigger.setTrigger(*trace);
        emit dataChanged(idx, idx, { Qt::DisplayRole, Qt::EditRole, DataSortRole });
        return true;
      }
    }
    break;
  case COL_COLOUR:
    if (role == Qt::EditRole)
    {
      QColor color = value.value<QColor>();
      if (color.isValid())
      {
        trace->setColor(color);
        emit dataChanged(idx, idx, { Qt::BackgroundRole, Qt::DisplayRole, Qt::EditRole, DataSortRole });
        return true;
      }
    }
    break;

  case COL_TRIGGER:
    if (role == Qt::CheckStateRole)
    {
      if (trace->isValid())
      {
        m_trigger.setTrigger(*trace);
        emit dataChanged(index(0, COL_TRIGGER), index(rowCount() - 1, COL_TRIGGER), { Qt::CheckStateRole, DataSortRole });
        return true;
      }
    }
    break;

  case COL_LABEL:
    if (role == Qt::EditRole)
    {
      trace->setLabel(value.toString());
      emit dataChanged(idx, idx, { Qt::DisplayRole, Qt::EditRole, DataSortRole });
      return true;
    }
  }

  return false;
}

void ScopeModel::removeAllTraces()
{
  beginResetModel();
  private_removeAllTraces();
  endResetModel();
}

void ScopeModel::private_removeAllTraces()
{
  stop();
  m_traceLookup.clear();

  for (ScopeTrace* trace : m_traceTable)
    delete trace;

  m_traceTable.clear();

  // Set extents back to default
  m_endTime = 0;
  setMaxValue(kMaxDmx8);
}

void ScopeModel::clearValues()
{
  // Clear all the trace values
  for (ScopeTrace* trace : m_traceTable)
  {
    trace->clear();
  }

  // Reset time extents
  m_startOffset = 0;
  m_endTime = 0;
  m_wallclockTrigger_ms = 0;
}

QString ScopeModel::captureConfigurationString() const
{
  QString result = (m_storeAllPoints ? QLatin1String("All Packets,") : QLatin1String("Level Changes,"));

  result.append(m_trigger.configurationString());
  result.append(QLatin1Char(','));

  if (m_runTime > 0)
    result.append(QStringLiteral("Run For %1 sec,").arg(m_runTime));

  result.chop(1);
  return result;
}

void ScopeModel::setCaptureConfiguration(const QString& configString)
{
  if (configString.isEmpty())
    return;

  m_storeAllPoints = configString.contains(QLatin1String("All Packets"), Qt::CaseInsensitive);

  m_trigger.setConfiguration(configString);

  qsizetype runTimePos = configString.indexOf(QLatin1String("Run For"));
  const qsizetype runTimeSecPos = configString.indexOf(QLatin1String("sec"), runTimePos);
  if (runTimePos < 0 || runTimeSecPos < 0)
  {
    m_runTime = 0;
  }
  else
  {
    bool ok;
    runTimePos += 8;
    const qreal time = configString.mid(runTimePos, runTimeSecPos - runTimePos).toDouble(&ok);
    if (ok)
      m_runTime = time;
  }
}

bool ScopeModel::saveTraces(QIODevice& file) const
{
  if (!file.isWritable())
    return false;

  // Cannot write while running as would block the receive threads
  if (isRunning())
    return false;

  QTextStream out(&file);
  out.setCodec("UTF-8");
  out.setLocale(QLocale::c());
  out.setRealNumberNotation(QTextStream::FixedNotation);
  out.setRealNumberPrecision(3);

  // Table:
  // Capture Options:,All Packets/Level Changes
  // 
  //              Labels, bob, jim, ...
  // 2024-01-15,   Color, red, green, ...
  // Wallclock,   Time (s),U1.1, U1.2/3, ... (Given as Universe.CoarseDMX/FineDmx (1-512)
  // 12:00:00.000, 0.000, 255,    0, ...
  // 12:00:00.020, 0.020, 128,  128, ...
  // 12:00:00.040, 0.040, 127,  255, ...

  // Export capture configuration line
  out << CaptureOptionsTitle << QLatin1String(":,") << captureConfigurationString() << QStringLiteral(",hh:mm:ss.000");
  out << "\n\n";

  // First row time
  float this_row_time = std::numeric_limits<float>::max();

  // Iterators for each trace
  using ValueIterator = std::vector<QVector2D>::const_iterator;
  // Copy the values
  struct ValueItem
  {
    ValueItem(const ScopeTrace* trace) : values(trace->values().value())
    {
      current = values.begin();
    }
    const std::vector<QVector2D> values;
    ValueIterator current;
  };
  std::vector<ValueItem> traces_values;
  traces_values.reserve(rowCount());

  QDateTime datetime = QDateTime::currentDateTime();

  QString label_header = QStringLiteral(",") + RowTitleLabel;

  QString color_header;
  if (asWallclockTime(datetime, 0.0))
  {
    color_header = datetime.toString(DateTimeFormatString);
  }
  color_header = color_header + QStringLiteral(",") + RowTitleColor;

  QString name_header = ColumnTitleWallclockTime + QStringLiteral(",") + ColumnTitleTimestamp;

  // Header rows sorted by universe
  for (const auto& universe : m_traceLookup)
  {
    for (const ScopeTrace* trace : universe.second)
    {
      // Get the values for this column
      traces_values.emplace_back(trace);

      // Assemble the label header string
      label_header.append(QLatin1Char(','));
      label_header.append(trace->label());

      // Assemble the color header string
      color_header.append(QLatin1Char(','));
      color_header.append(trace->color().name());

      // Assemble the name header string
      name_header.append(QLatin1String(", "));
      name_header.append(trace->universeAddressString());

      // Find the first and last row timestamps
      if (!traces_values.back().values.empty())
      {
        if (traces_values.back().values.front()[0] < this_row_time)
          this_row_time = traces_values.back().values.front()[0];
      }
    }
  }
  out << label_header << '\n'
    << color_header << '\n'
    << name_header;

  // Value rows
  while (this_row_time < std::numeric_limits<float>::max())
  {
    // Start new row and output timestamp
    QString wallclock;
    if (asWallclockTime(datetime, this_row_time))
    {
      wallclock = datetime.toString(TimeFormatString);
    }
    out << '\n' << wallclock << ',' << this_row_time;
    float next_row_time = std::numeric_limits<float>::max();

    for (auto& value_its : traces_values)
    {
      // Next Column
      out << ',';
      if (value_its.current != value_its.values.end())
      {
        // This column has a value for this time
        if (qFuzzyCompare(this_row_time, (*value_its.current)[0]))
        {
          // Output a value for this timestamp and step forward
          out << static_cast<int>((*value_its.current)[1]);
          ++value_its.current;
        }
        // Determine the next row time
        if (value_its.current != value_its.values.end() && (*value_its.current)[0] < next_row_time)
        {
          next_row_time = (*value_its.current)[0];
        }
      }
    }
    // On to the next row
    this_row_time = next_row_time;
  }

  return true;
}

struct TitleRows {
  QString config;
  QString labels;
  QString colors;
  QString universes;
};

TitleRows FindUniverseTitles(QTextStream& in)
{
  TitleRows result;
  // Find start of data
  QString line;
  while (in.readLineInto(&line))
  {
    if (line.startsWith(CaptureOptionsTitle))
    {
      result.config = line;
    }
    else if (line.contains(RowTitleLabel))
    {
      // The last label line
      result.labels = line;
    }
    else if (line.contains(RowTitleColor))
    {
      // Probably the title line
      result.colors = line;
      result.universes = in.readLine();
      if (result.universes.startsWith(ColumnTitleTimestamp) || result.universes.startsWith(ColumnTitleWallclockTime))
      {
        return result;
      }
      return TitleRows();  // Failed

    }
  }
  return TitleRows();
}

bool ScopeModel::loadTraces(QIODevice& file)
{
  if (!file.isReadable())
    return false;

  QTextStream in(&file);
  in.setCodec("UTF-8");
  in.setLocale(QLocale::c());
  in.setRealNumberNotation(QTextStream::FixedNotation);
  in.setRealNumberPrecision(3);

  const auto title_line = FindUniverseTitles(in);
  if (title_line.universes.isEmpty())
    return false;

  // Split the title lines to find the trace colors and names
  auto labels = title_line.labels.splitRef(QLatin1Char(','), Qt::KeepEmptyParts);
  auto colors = title_line.colors.splitRef(QLatin1Char(','), Qt::KeepEmptyParts);
  auto titles = title_line.universes.splitRef(QLatin1Char(','), Qt::KeepEmptyParts);

  // Find data columns
  int timeColumn = -1; // Column index for time offset
  QDateTime wallclockTrigger;
  // Data is always in the column after time offset
  for (int i = 0; timeColumn == -1 && i < colors.size(); ++i)
  {
    if (colors.isEmpty())
      return false;
    if (titles.isEmpty())
      return false;

    // Grab the zero datetime from Colors
    if (titles.front() == ColumnTitleWallclockTime)
      wallclockTrigger = QDateTime::fromString(colors.front().toString(), DateTimeFormatString);
    else if (titles.front() == ColumnTitleTimestamp)
      timeColumn = i;

    // Remove the row header titles
    colors.pop_front();
    titles.pop_front();

    // Labels are optional
    if (!labels.isEmpty())
      labels.pop_front();
  }

  // Time is required
  if (timeColumn < 0)
    return false;

  const int firstTraceColumn = timeColumn + 1; // Column index of first trace

  // Remove empty colors from the end
  while (colors.last().isEmpty())
    colors.pop_back();
  // Remove empty items from the end
  while (titles.last().isEmpty())
    titles.pop_back();

  if (colors.size() != titles.size() || titles.empty())
    return false; // No or invalid data

  // Fairly likely to be valid, stop and clear my data now
  beginResetModel();
  private_removeAllTraces();

  // Read the wallclock trigger datetime
  if (wallclockTrigger.isValid())
    m_wallclockTrigger_ms = wallclockTrigger.toMSecsSinceEpoch();
  else // Or set it to a midnight
    m_wallclockTrigger_ms = QDateTime::fromString(QStringLiteral("1975-01-01 00:00:00.000"), DateTimeFormatString).toMSecsSinceEpoch();

  struct UnivSlots
  {
    uint16_t universe = 0;
    uint16_t address_hi = 0;
    uint16_t address_lo = 0;
  };
  std::vector<UnivSlots> trace_idents;

  for (qsizetype i = 0; i < titles.size(); ++i)
  {
    // Grab color
    const QColor color(colors[i]);

    // Find universe and patch
    const auto& full_title = titles[i];

    UnivSlots univ_slots;
    if (!ScopeTrace::extractUniverseAddress(full_title.trimmed(), univ_slots.universe, univ_slots.address_hi, univ_slots.address_lo))
    {
      trace_idents.push_back(UnivSlots());  // Skip this column
      continue;
    }

    const QString label = labels.size() > i ? labels.at(i).toString() : QString();
    if (addTrace(label, color, univ_slots.universe, univ_slots.address_hi, univ_slots.address_lo) == AddResult::Added)
    {
      trace_idents.push_back(univ_slots);
    }
    else
      trace_idents.push_back(UnivSlots());
  }

  // Now have all the trace idents and the container will not change
  // Get all the pointers in order with null for bad columns
  std::vector<ScopeTrace*> traces;
  for (const auto& ident : trace_idents)
  {
    traces.push_back(findTrace(ident.universe, ident.address_hi, ident.address_lo));
  }

  QString data_line;
  float prev_timestamp = 0;
  while (in.readLineInto(&data_line))
  {
    if (data_line.isEmpty())
      continue;

    const auto data = data_line.splitRef(QLatin1Char(','), Qt::KeepEmptyParts);
    // Ignore any lines that do not have a column for all traces
    if (data.size() < traces.size() + firstTraceColumn)
      continue;

    // Time moves ever forward. Ignore any lines in the past
    bool ok = false;
    const float timestamp = data[timeColumn].toFloat(&ok);
    if (!ok || prev_timestamp > timestamp)
      continue;

    prev_timestamp = timestamp;

    for (size_t i = 0; i < traces.size(); ++i)
    {
      ScopeTrace* trace = traces[i];
      if (trace)
      {
        bool ok = false;
        const float level = data[i + firstTraceColumn].toFloat(&ok);
        if (ok)
          trace->addValue({ timestamp, level });
      }
    }
  }

  m_endTime = prev_timestamp;

  // Finally, update the config
  setCaptureConfiguration(title_line.config);

  endResetModel();
  // Force a re-render
  emit traceVisibilityChanged();
  return true;
}

bool ScopeModel::listeningToUniverse(uint16_t universe) const
{
  return m_traceLookup.count(universe) != 0;
}

void ScopeModel::start()
{
  if (isRunning())
    return;

  // Clear all values and start the clock
  // Cannot permit time to go backwards
  clearValues();

  m_running = true;

  // Set up the Trigger or start immediately
  if (m_trigger.isTrigger())
  {
    m_trigger.last_level = -1;
  }
  else
  {
    triggerNow(sACNManager::secsElapsed());
  }

  for (const auto& universe : m_traceLookup)
  {
    addListener(universe.first);
  }

  emit runningChanged(true);
}

void ScopeModel::stop()
{
  if (!isRunning())
    return;

  m_running = false;
  if (m_timerId != 0)
  {
    killTimer(m_timerId);
    m_timerId = 0;
  }


  // Disconnect all
  for (auto& listener : m_listeners)
  {
    listener->removeDirectCallback(this);
  }
  // And clear/shutdown
  m_listeners.clear();
  emit runningChanged(false);
}

void ScopeModel::setRunTime(qreal seconds)
{
  if (runTime() == seconds)
    return;

  // Validate and emit new or existing value
  if (seconds >= 0)
    m_runTime = seconds;

  emit runTimeChanged(runTime());
}

// Triggers
void ScopeModel::setTriggerType(Trigger mode)
{
  m_trigger.mode = mode;
  emit traceVisibilityChanged();
}

void ScopeModel::setTriggerLevel(uint16_t level)
{
  m_trigger.level = level;
  emit traceVisibilityChanged();
}

bool ScopeModel::moveTrace(ScopeTrace* trace, uint16_t new_universe, bool clear_values)
{
  const uint16_t old_universe = trace->universe();

  if (!trace->setUniverse(new_universe, clear_values))
    return false; // New universe invalid or unchanged

  // Remove it from the old lookup
  removeFromLookup(trace, old_universe);

  // Add the ScopeTrace to the new universe in map
  auto univ_it = m_traceLookup.find(new_universe);
  if (univ_it == m_traceLookup.end())
  {
    // First trace on this universe
    m_traceLookup.emplace(new_universe, std::vector<ScopeTrace*>(1, trace));

    // Maybe start listening
    if (isRunning())
      addListener(new_universe);
  }
  else
  {
    // Add this new trace to the existing universe
    auto& univs_item = univ_it->second;
    univs_item.push_back(trace);
  }
  return true;
}

void ScopeModel::removeFromLookup(ScopeTrace* trace, uint16_t old_universe)
{
  // Remove from old universe
  auto univ_it = m_traceLookup.find(old_universe);
  if (univ_it != m_traceLookup.end())
  {
    // Remove from old Universe
    auto& univ_item = univ_it->second;
    for (auto it = univ_item.begin(); it != univ_item.end(); /**/)
    {
      if ((*it) == trace)
      {
        it = univ_item.erase(it);
      }
      else
      {
        ++it;
      }
    }

    // If was last trace on a universe, stop listening
    if (univ_item.empty())
    {
      for (auto it = m_listeners.begin(); it != m_listeners.end(); /**/)
      {
        if ((*it)->universe() == old_universe)
        {
          // Disconnect signal
          (*it)->removeDirectCallback(this);
          it = m_listeners.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
  }
}

void ScopeModel::addListener(uint16_t universe)
{
  auto listener = sACNManager::Instance().getListener(universe);
  listener->addDirectCallback(this);
  m_listeners.push_back(listener);
}

void ScopeModel::updateMaxValue()
{
  for (const ScopeTrace* trace : m_traceTable)
  {
    if (trace->isSixteenBit())
    {
      setMaxValue(kMaxDmx16);
      return;
    }
  }
  setMaxValue(kMaxDmx8);
}

void ScopeModel::setMaxValue(qreal maxValue)
{
  if (m_maxValue == maxValue)
    return;
  m_maxValue = maxValue;
  maxValueChanged();
}

void ScopeModel::triggerNow(qreal offset)
{
  {
    // Determine approximate offset to wallclock time by grabbing both
    const qint64 now_ms = sACNManager::nsecsElapsed() / 1000000;
    const qint64 nowWallclock_ms = QDateTime::currentMSecsSinceEpoch();
    m_wallclockTrigger_ms = (nowWallclock_ms - now_ms) + (offset / 1000.0);
  }

  m_startOffset = offset;
  // Update the offsets of all traces
  for (ScopeTrace* trace : m_traceTable)
  {
    trace->applyOffset(offset);
  }
  // Reset the counters
  for (sACNManager::tListener& listener : m_listeners)
  {
    auto sources = listener->getSourceList();
    for (sACNSource* source : sources)
    {
      source->resetSeqErr();
      source->resetJumps();
      source->fpscounter.ClearHistogram();
    }
  }

  emit queueTriggered();
}

QRectF ScopeModel::traceExtents() const
{
  return QRectF(0, 0, m_endTime, m_maxValue);
}

qreal ScopeModel::endTime() const
{
  if (isTriggered())
    return (qreal(sACNManager::elapsed()) / 1000) - m_startOffset;

  return m_endTime;
}

bool ScopeModel::asWallclockTime(QDateTime& datetime, qreal time) const
{
  if (m_wallclockTrigger_ms == 0)
    return false;

  datetime.setMSecsSinceEpoch(m_wallclockTrigger_ms + ((time + m_startOffset) * 1000));
  return true;
}

void ScopeModel::sACNListenerDmxReceived(tock packet_tock, int universe, const sACNMergedSourceList& levels, const sACNSource* source)
{
  if (!m_running)
    return;

  // Find traces for universe
  auto it = m_traceLookup.find(universe);
  if (it == m_traceLookup.end())
    return;

  const qreal timestamp = std::chrono::duration<qreal>(packet_tock.Get()).count();

  if (!isTriggered())
  {
    // Only store one level for each trace until the trigger is fired
    for (ScopeTrace* trace : it->second)
    {
      trace->setFirstPoint(timestamp, levels, source);
    }

    uint16_t current_level;
    if (universe == m_trigger.universe && fillValue(current_level, m_trigger.address_hi - 1, m_trigger.address_lo - 1, levels, source))
    {
      // Have a current level, maybe start the clock
      switch (m_trigger.mode)
      {
      default: break;
      case Trigger::Above:
        if (current_level > m_trigger.level)
          triggerNow(timestamp);
        break;
      case Trigger::Below:
        if (current_level < m_trigger.level)
          triggerNow(timestamp);
        break;
      case Trigger::LevelCross:
        if ((m_trigger.last_level == m_trigger.level && current_level != m_trigger.level) // Was at, now not
          || (m_trigger.last_level > m_trigger.level && current_level < m_trigger.level) // Was above, now below
          || (m_trigger.last_level != -1 && m_trigger.last_level < m_trigger.level && current_level > m_trigger.level) // Was below, now above
          )
          triggerNow(timestamp);
        break;
      }
      m_trigger.last_level = current_level;
    }
    return;
  }

  // Time in seconds
  {
    m_endTime = timestamp - m_startOffset;
    if (m_runTime > 0 && m_endTime >= m_runTime)
    {
      emit queueStop();
      return;
    }
  }

  for (ScopeTrace* trace : it->second)
  {
    trace->addPoint(m_endTime, levels, source, m_storeAllPoints);
  }
}

void ScopeModel::onQueueTriggered()
{
  if (m_runTime > 0)
  {
    // Stop myself 10ms after the end of the runtime to ensure queued packets get processed
    // May create a 1 packet jitter in counts but this is acceptable
    m_timerId = startTimer(int(m_runTime * 1000.0) + 10, Qt::PreciseTimer);
  }
}

void ScopeModel::timerEvent(QTimerEvent* ev)
{
  if (ev->timerId() == m_timerId)
    stop();
}

GlScopeWidget::GlScopeWidget(QWidget* parent)
  : QOpenGLWidget(parent)
{
  setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

  m_model = new ScopeModel(this);
  connect(m_model, &ScopeModel::runningChanged, this, &GlScopeWidget::onRunningChanged);
  connect(m_model, &ScopeModel::traceVisibilityChanged, this, QOverload<void>::of(&QOpenGLWidget::update));

  setMinimumSize(200, 200);
  setVerticalScaleMode(VerticalScale::Percent);
  setScopeView();
}

GlScopeWidget::~GlScopeWidget()
{
  cleanupGL();
}

void GlScopeWidget::setVerticalScaleMode(VerticalScale scaleMode)
{
  switch (scaleMode)
  {
  default:
    return; // Invalid, do nothing
  case VerticalScale::Percent:
  {
    m_levelInterval = 10;
    m_scopeView.setBottom(kMaxDmx8);  // The 16 bit matrix downscales to 8bit
  } break;
  case VerticalScale::Dmx8:
  {
    m_levelInterval = 20;
    m_scopeView.setBottom(kMaxDmx8);
  } break;
  case VerticalScale::Dmx16:
  {
    m_levelInterval = 10000;
    m_scopeView.setBottom(kMaxDmx16);
  } break;
  case VerticalScale::DeltaTime:
  {
    m_levelInterval = 100;
    m_scopeView.setBottom(E131_DATA_KEEP_ALIVE_INTERVAL_MAX);
  } break;
  }

  m_verticalScaleMode = scaleMode;

  updateMVPMatrix();
  update();
}


bool GlScopeWidget::levelInView(const QPointF& point) const
{
  return (point.y() <= m_scopeView.bottom() && point.y() >= m_scopeView.top());
}

void GlScopeWidget::setScopeView(const QRectF& rect)
{
  if (rect.isEmpty())
  {
    m_scopeView = m_model->traceExtents();
    m_scopeView.setRight(m_defaultIntervalCount * m_timeInterval);
    setVerticalScaleMode(m_verticalScaleMode);
  }
  else if (rect == m_scopeView)
  {
    return;
  }
  else
  {
    m_scopeView = rect;
  }

  updateMVPMatrix();
  update();
}

void GlScopeWidget::setTimeDivisions(int milliseconds)
{
  if (timeDivisions() == milliseconds)
    return;

  // Must not go lower than 1ms
  if (milliseconds < 1)
    milliseconds = 1;

  m_timeInterval = static_cast<qreal>(milliseconds) / 1000.0;
  m_scopeView.setWidth(m_timeInterval * m_defaultIntervalCount);
  updateMVPMatrix();
  update();

  emit timeDivisionsChanged(milliseconds);
}

void GlScopeWidget::setTimeFormat(TimeFormat format)
{
  if (format == m_timeFormat)
    return;

  m_timeFormat = format;
  update();

  onRunningChanged(m_model->isRunning());

  emit timeFormatChanged();
}

void GlScopeWidget::setDotSize(float width)
{
  if (width == m_levelDotSize)
    return;

  m_levelDotSize = width;
  update();

  emit dotSizeChanged(m_levelDotSize);
}

void GlScopeWidget::ShaderProgram::BuildProgram(const char* shaderName, const char* vertexShaderSource, const char* fragmentShaderSource)
{
  program = new QOpenGLShaderProgram();

  if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource))
  {
    qDebug() << shaderName << "Vertex Shader Failed:" << program->log();
    UnloadProgram();
    return;
  }

  if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource))
  {
    qDebug() << shaderName << "Fragment Shader Failed:" << program->log();
    UnloadProgram();
    return;
  }

  if (!program->link())
  {
    qDebug() << shaderName << "Shader Link Failed:" << program->log();
    UnloadProgram();
    return;
  }

  vertexLocation = program->attributeLocation("vertex");
  matrixUniform = program->uniformLocation("mvp");
  colorUniform = program->uniformLocation("color");
  pointSizeUniform = program->uniformLocation("pointsize");

  if (!IsValid())
    UnloadProgram();
}

void GlScopeWidget::ShaderProgram::UnloadProgram()
{
  if (program == nullptr)
    return;

  delete program;
  program = nullptr;
}

bool GlScopeWidget::ShaderProgram::IsValid() const
{
  return program != nullptr && vertexLocation != -1 && matrixUniform != -1 && colorUniform != -1 && pointSizeUniform != -1;
}

bool GlScopeWidget::ShaderProgram::bind()
{
  return program && program->bind();
}

void GlScopeWidget::ShaderProgram::release()
{
  program->release();
}

void GlScopeWidget::ShaderProgram::setMatrix(const QMatrix4x4& matrix)
{
  program->setUniformValue(matrixUniform, matrix);
}

void GlScopeWidget::ShaderProgram::setColor(const QColor& color)
{
  program->setUniformValue(colorUniform, color);
}

void GlScopeWidget::ShaderProgram::setPointSize(float size)
{
  program->setUniformValue(pointSizeUniform, size);
}

void GlScopeWidget::initializeGL()
{
  // Reparenting to a different top-level window causes the OpenGL Context to be destroyed and recreated
  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GlScopeWidget::cleanupGL);

  initializeOpenGLFunctions();

  glClearColor(0, 0, 0, 1);

  // Compile shaders
  // 2D passthrough shader
  const char* vertexShaderSource =
    "in vec2 vertex;\n"
    "uniform mat4 mvp;\n"
    "uniform float pointsize;\n"
    "void main()\n"
    "{\n"
    "  gl_Position = mvp * vec4(vertex.x, vertex.y, 0, 1.0);\n"
    "  gl_PointSize = pointsize;"
    "}\n";

  // Delta Time shader
  const char* vertexDeltaTimeShaderSource =
    "in vec4 vertex;\n" // { x prev_timestamp, y prev_level, z timestamp, w level }
    "uniform mat4 mvp;\n"
    "uniform float pointsize;\n"
    "void main()\n"
    "{\n"
    //"  gl_Position = mvp * vec4(vertex.z, 127, 0, 1.0);\n"
    "  gl_Position = mvp * vec4(vertex.z, (vertex.z - vertex.x) * 1000, 0, 1.0);\n"
    "  gl_PointSize = pointsize;"
    "}\n";

  const char* fragmentShaderSource =
    "uniform vec4 color;\n"
    "void main()\n"
    "{\n"
    "  gl_FragColor = color;\n"
    "}\n";

  m_xyProgram.BuildProgram("xy", vertexShaderSource, fragmentShaderSource);
  m_deltaProgram.BuildProgram("deltaTime", vertexDeltaTimeShaderSource, fragmentShaderSource);
}

void GlScopeWidget::cleanupGL()
{
  if (!m_xyProgram.IsValid())
    return;

  // The context is about to be destroyed, it may be recreated later
  makeCurrent();

  m_xyProgram.UnloadProgram();
  m_deltaProgram.UnloadProgram();

  doneCurrent();
}

inline static void DrawLevelAxisText(QPainter& painter, const QFontMetricsF& metrics, const QRectF& scopeWindow, int level, qreal y_scale, const QString& postfix)
{
  const qreal y = scopeWindow.height() - (level * y_scale);

  // TODO: use QStaticText to optimise the text layout
  const QString text = QString::number(level) + postfix;

  QRectF fontRect = metrics.boundingRect(text);
  fontRect.moveBottomRight(QPointF(-AXIS_TICK_SIZE, y + (fontRect.height() / 2.0)));

  painter.drawText(fontRect, text, QTextOption(Qt::AlignLeft));
}

void GlScopeWidget::paintGL()
{
  const qreal endTime = m_model->endTime();
  if (m_followNow)
  {
    if (endTime > m_scopeView.width())
    {
      m_scopeView.moveRight(endTime);
      updateMVPMatrix();
    }
  }

  glClear(GL_COLOR_BUFFER_BIT);

  // Pixel-perfect grid lines
  std::vector<QVector2D> gridLines;
  gridLines.reserve((11 + 14) * 2); // 10 ticks across, 13 up for 0-255

  // Cursor lines
  std::vector<QVector2D> cursorLines;
  cursorLines.reserve(4);

  // Colors
  static const QColor gridColor(0x43, 0x43, 0x43);
  static const QColor textColor(Qt::white);
  static const QColor timeCursorColor(Qt::white);
  static const QColor cursorColor(Qt::gray);
  static const QColor cursorTextColor(Qt::white);
  static const QColor triggerCursorColor(Qt::gray);

  // Origin is the bottom left of the scope window
  QPointF origin(rect().bottomLeft().x() + AXIS_LABEL_WIDTH, rect().bottomLeft().y() - AXIS_LABEL_HEIGHT);

  QRectF scopeWindow;
  scopeWindow.setBottomLeft(origin);
  scopeWindow.setTop(rect().top() + TOP_GAP);
  scopeWindow.setRight(rect().right() - RIGHT_GAP);

  // Draw nothing if tiny
  if (scopeWindow.width() < AXIS_TICK_SIZE)
    return;

  const qreal x_scale = scopeWindow.width() / m_scopeView.width();

  QFont font;
  QFontMetricsF metrics(font);
  {
    // Draw the labels using QPainter as is easiest way to get the text
    // It's ok if the labels aren't pixel perfect

    QPen textPen;
    textPen.setColor(textColor);

    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(textPen);

    painter.translate(scopeWindow.topLeft().x(), scopeWindow.topLeft().y());

    // Draw vertical (level) axis
    {
      const float lineLeft = m_scopeView.left();
      const float lineRight = std::fmaxf(m_scopeView.right(), endTime);

      // Cursor if active
      if (!m_cursorPoint.isNull())
      {
        // Time cursor
        if (timeInView(m_cursorPoint))
        {
          cursorLines.emplace_back(static_cast<float>(m_cursorPoint.x()), 0.0f);
          cursorLines.emplace_back(static_cast<float>(m_cursorPoint.x()), m_scopeView.bottom());
        }

        // Level cursor
        if (levelInView(m_cursorPoint))
        {
          cursorLines.emplace_back(lineLeft, static_cast<float>(m_cursorPoint.y()));
          cursorLines.emplace_back(lineRight, static_cast<float>(m_cursorPoint.y()));
        }
      }

      QString postfix;
      switch (m_verticalScaleMode)
      {
      case VerticalScale::Percent:
      {
        postfix = QStringLiteral("%");
        const qreal max_value = 100.0;
        const qreal y_scale = scopeWindow.height() / 100.0; // Percent
        const float value_scale = kMaxDmx8 / 100.0f;

        // From bottom to top
        for (qreal value = m_scopeView.top(); value < max_value; value += m_levelInterval)
        {
          // Grid lines in trace space
          gridLines.emplace_back(lineLeft, static_cast<float>(value) * value_scale);
          gridLines.emplace_back(lineRight, static_cast<float>(value) * value_scale);
          DrawLevelAxisText(painter, metrics, scopeWindow, value, y_scale, postfix);
        }

        // Final row, may be uneven
        // Grid lines in trace space
        gridLines.emplace_back(lineLeft, static_cast<float>(m_scopeView.bottom()));
        gridLines.emplace_back(lineRight, static_cast<float>(m_scopeView.bottom()));
        DrawLevelAxisText(painter, metrics, scopeWindow, max_value, y_scale, postfix);
      } break;

      case VerticalScale::DeltaTime:
        postfix = QStringLiteral("ms");
        [[fallthrough]];
      case VerticalScale::Dmx8:
      case VerticalScale::Dmx16:
      {
        const int max_value = m_scopeView.bottom();
        const qreal y_scale = scopeWindow.height() / m_scopeView.bottom();

        // From bottom to top
        for (int value = m_scopeView.top(); value < max_value; value += m_levelInterval)
        {
          // Grid lines in trace space
          gridLines.emplace_back(lineLeft, static_cast<float>(value));
          gridLines.emplace_back(lineRight, static_cast<float>(value));
          DrawLevelAxisText(painter, metrics, scopeWindow, value, y_scale, postfix);
        }

        // Final row, may be uneven
        // Grid lines in trace space
        gridLines.emplace_back(lineLeft, max_value);
        gridLines.emplace_back(lineRight, max_value);
        DrawLevelAxisText(painter, metrics, scopeWindow, max_value, y_scale, postfix);
      } break;
      }
    }

    // Draw horizontal (time) axis
    painter.resetTransform();
    painter.translate(scopeWindow.bottomLeft().x(), scopeWindow.bottomLeft().y());

    if (m_timeFormat == TimeFormat::Elapsed)
    {
      const bool milliseconds = (m_timeInterval < 1.0);
      for (qreal time = roundCeilMultiple(m_scopeView.left(), m_timeInterval); time < m_scopeView.right() + 0.001; time += m_timeInterval)
      {
        // Grid lines in trace space
        gridLines.emplace_back(static_cast<float>(time), 0.0f);
        gridLines.emplace_back(static_cast<float>(time), m_scopeView.bottom());

        const qreal x = (time - m_scopeView.left()) * x_scale;

        const QString text = milliseconds ? QStringLiteral("%1ms").arg(time * 1000.0) : QStringLiteral("%1s").arg(time);
        QRectF fontRect = metrics.boundingRect(text);
        fontRect.moveCenter(QPointF(x, AXIS_LABEL_HEIGHT / 2.0));
        painter.drawText(fontRect, text, QTextOption(Qt::AlignLeft));
      }
    }
    else
    {
      QDateTime datetime = QDateTime::currentDateTime();

      for (qreal time = roundCeilMultiple(m_scopeView.left(), m_timeInterval); time < m_scopeView.right() + 0.001; time += m_timeInterval)
      {
        // Grid lines in trace space
        gridLines.emplace_back(static_cast<float>(time), 0.0f);
        gridLines.emplace_back(static_cast<float>(time), m_scopeView.bottom());

        const qreal x = (time - m_scopeView.left()) * x_scale;

        if (!m_model->asWallclockTime(datetime, time))
        {
          // Add time interval
          datetime = datetime.addMSecs(m_timeInterval * 1000);
        }

        const QString text = x_scale > 80.0 ? datetime.toString(TimeFormatString) : datetime.toString(ShortTimeFormatString);

        QRectF fontRect = metrics.boundingRect(text);
        fontRect.moveCenter(QPointF(x, AXIS_LABEL_HEIGHT / 2.0));
        painter.drawText(fontRect, text, QTextOption(Qt::AlignLeft));
      }
    }
  }

  // Unable to render at all
  if (!m_xyProgram.IsValid())
    return;

  m_xyProgram.bind();
  glEnableVertexAttribArray(m_xyProgram.vertexLocation);

  m_xyProgram.setMatrix(m_mvpMatrix);

  // Draw the gridlines
  {
    m_xyProgram.setColor(gridColor);
    glVertexAttribPointer(m_xyProgram.vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, gridLines.data());
    glDrawArrays(GL_LINES, 0, gridLines.size());
  }

  // Draw the cursorLines
  if (!cursorLines.empty())
  {
    m_xyProgram.setColor(cursorColor);
    glVertexAttribPointer(m_xyProgram.vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, cursorLines.data());
    glDrawArrays(GL_LINES, 0, cursorLines.size());
  }

  if (m_model->isTriggered())
  {
    // Draw the current time
    m_xyProgram.setColor(timeCursorColor);
    const std::vector<QVector2D> nowLine = {
      {static_cast<float>(m_model->endTime()), 0},
      {static_cast<float>(m_model->endTime()), static_cast<float>(m_scopeView.bottom())}
    };
    glVertexAttribPointer(m_xyProgram.vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, nowLine.data());
    glDrawArrays(GL_LINE_STRIP, 0, 2);
  }
  else if (!m_model->triggerIsFreeRun())
  {
    // Draw the trigger level marker
    m_xyProgram.setColor(triggerCursorColor);

    const std::vector<QVector2D> triggerLine = makeTriggerLine(m_model->triggerType());

    glVertexAttribPointer(m_xyProgram.vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, triggerLine.data());
    glDrawArrays(GL_TRIANGLES, 0, triggerLine.size());
  }

  glDisableVertexAttribArray(m_xyProgram.vertexLocation);
  m_xyProgram.release();

  if (m_verticalScaleMode == VerticalScale::DeltaTime)
  {
    // Draw the trace using the deltatime program
    m_deltaProgram.bind();
    glEnableVertexAttribArray(m_deltaProgram.vertexLocation);

    m_deltaProgram.setMatrix(m_mvpMatrix);

    glEnable(GL_PROGRAM_POINT_SIZE);
    m_deltaProgram.setPointSize(m_levelDotSize * float(devicePixelRatioF()));
    for (ScopeTrace* trace : m_model->traces())
    {
      if (!trace->enabled())
        continue;

      m_deltaProgram.setColor(trace->color());
      const auto levels = trace->values();
      if (levels.value().size() > 1)
      {
        glVertexAttribPointer(m_deltaProgram.vertexLocation, 4, GL_FLOAT, GL_FALSE, sizeof(QVector2D), levels.value().data());
        glDrawArrays(GL_LINE_STRIP, 0, levels.value().size() - 1);

        // Draw the points if enabled
        if (m_levelDotSize > 0.5f)
        {
          glDrawArrays(GL_POINTS, 0, levels.value().size() - 1);
        }
      }
    }
    glDisable(GL_PROGRAM_POINT_SIZE);

    glDisableVertexAttribArray(m_deltaProgram.vertexLocation);
    m_deltaProgram.release();
  }
  else
  {
    m_xyProgram.bind();
    glEnableVertexAttribArray(m_xyProgram.vertexLocation);

    m_xyProgram.setMatrix(m_mvpMatrix);

    glEnable(GL_PROGRAM_POINT_SIZE);
    m_xyProgram.setPointSize(m_levelDotSize * float(devicePixelRatioF()));
    for (ScopeTrace* trace : m_model->traces())
    {
      if (!trace->enabled())
        continue;

      // Change the scale as needed
      if (m_verticalScaleMode == VerticalScale::Percent)
      {
        m_xyProgram.setMatrix(trace->isSixteenBit() ? m_mvpMatrix16 : m_mvpMatrix);
      }

      m_xyProgram.setColor(trace->color());
      const auto levels = trace->values();
      glVertexAttribPointer(m_xyProgram.vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, levels.value().data());
      glDrawArrays(GL_LINE_STRIP, 0, levels.value().size());

      // Draw the points if enabled
      if (m_levelDotSize > 0.5f)
      {
        glDrawArrays(GL_POINTS, 0, levels.value().size());
      }
    }
    glDisable(GL_PROGRAM_POINT_SIZE);

    glDisableVertexAttribArray(m_xyProgram.vertexLocation);
    m_xyProgram.release();
  }

  // Cursor labels if active
  // Draw these last so the gridlines aren't on top
  if (!m_cursorPoint.isNull())
  {
    QPen textPen;
    textPen.setColor(cursorTextColor);

    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(textPen);

    painter.translate(scopeWindow.topLeft().x(), scopeWindow.topLeft().y());

    bool timeAtTop = true;

    // Cursor level as actual and percent if inside grid
    if (levelInView(m_cursorPoint))
    {
      // Actual value
      QString text = QString::number(m_cursorPoint.y(), 'f', 0);
      if (m_verticalScaleMode != VerticalScale::DeltaTime)
      {
        // Append the percentage
        const qreal percent_value = m_cursorPoint.y() * (m_verticalScaleMode == VerticalScale::Dmx16 ? 100.0f / kMaxDmx16 : 100.0f / kMaxDmx8);
        text.append(QStringLiteral(" (") + QString::number(percent_value, 'f', 2) + QStringLiteral("%)"));
      }
      else
      {
        text.append(QStringLiteral("ms"));
      }

      // Draw it above/below the line
      const qreal y_scale = scopeWindow.height() / m_scopeView.bottom();
      qreal y = scopeWindow.height() - (m_cursorPoint.y() * y_scale);
      if (m_cursorPoint.y() > m_scopeView.height() * 0.9)
      {
        y += metrics.ascent();
        timeAtTop = false;  // Draw the time at the bottom
      }
      else
      {
        y -= metrics.descent();
      }
      painter.drawText(QPointF(AXIS_TO_WINDOW_GAP, y), text);
    }

    // Cursor time as elapsed and wallclock (if available)
    if (timeInView(m_cursorPoint))
    {
      const bool milliseconds = (m_timeInterval < 1.0);
      QString text = milliseconds //
        ? QString::number(m_cursorPoint.x() * 1000.0, 'f', 0) + QStringLiteral("ms")
        : QString::number(m_cursorPoint.x(), 'f', 3) + QStringLiteral("s");
      QDateTime datetime = QDateTime::currentDateTime();
      if (m_model->asWallclockTime(datetime, m_cursorPoint.x()))
      {
        text += QStringLiteral(", ") + datetime.toString(TimeFormatString);
      }

      // Determine left/right of cursor
      qreal x = AXIS_TO_WINDOW_GAP + (m_cursorPoint.x() - m_scopeView.left()) * x_scale;
      const qreal textWidth = metrics.width(text);
      if (x + textWidth > scopeWindow.width())
        x -= (textWidth + 2.0 * AXIS_TO_WINDOW_GAP);

      painter.drawText(QPointF(x, timeAtTop ? metrics.height() : scopeWindow.height() - AXIS_TO_WINDOW_GAP), text);
    }
  }
}

void GlScopeWidget::resizeGL(int w, int h)
{
  // Pixel-to-pixel projection
  m_viewMatrix.setToIdentity();
  m_viewMatrix.ortho(0, w, 0, h, -1.0f, 1.0f);

  updateMVPMatrix();
}

void GlScopeWidget::timerEvent(QTimerEvent* /*ev*/)
{
  // Schedule an update
  update();
}

void GlScopeWidget::mousePressEvent(QMouseEvent* ev)
{
  if (ev->buttons() & Qt::LeftButton)
    updateCursor(ev->pos());

  QOpenGLWidget::mousePressEvent(ev);
}

void GlScopeWidget::mouseMoveEvent(QMouseEvent* ev)
{
  if (ev->buttons() & Qt::LeftButton)
  {
    updateCursor(ev->pos());
  }
  QOpenGLWidget::mouseMoveEvent(ev);
}

void GlScopeWidget::updateCursor(const QPoint& widgetPos)
{
  QPointF pos(widgetPos.x(), height() - widgetPos.y());

  // Be a little forgiving about pixel perfection to make it easier to get a line at the far left
  if (pos.x() < AXIS_LABEL_WIDTH && pos.x() > AXIS_LABEL_WIDTH - AXIS_TICK_SIZE)
    pos.setX(AXIS_LABEL_WIDTH);

  // Disable cursor when clicking out-of-bounds bottom-left
  if (pos.x() < AXIS_LABEL_WIDTH && pos.y() < AXIS_LABEL_HEIGHT)
  {
    m_cursorPoint = QPointF();
  }
  else
  {
    // Convert to trace space
    const QMatrix4x4 invModelMatrix = m_modelMatrix.inverted();
    m_cursorPoint = invModelMatrix * pos;
    // Round the level to nearest whole DMX
    m_cursorPoint.setY(std::round(m_cursorPoint.y()));
  }

  update();
}

void GlScopeWidget::onRunningChanged(bool running)
{
  // Must update the timescale when displaying wallclock time
  if (running || (m_timeFormat == TimeFormat::Wallclock && !m_model->isTriggered()))
  {
    if (m_renderTimer == 0)
    {
      // Redraw at the screen refresh or 5fps, whichever is greater
      const qreal framerate = screen()->refreshRate();
      m_renderTimer = startTimer(framerate > 5.0 ? static_cast<int>(std::ceil(1000.0 / framerate)) : 200);
    }
  }
  else
  {
    killTimer(m_renderTimer);
    m_renderTimer = 0;
  }
}

void GlScopeWidget::updateMVPMatrix()
{
  m_modelMatrix = QMatrix4x4();

  // Translate to origin
  m_modelMatrix.translate(AXIS_LABEL_WIDTH, AXIS_LABEL_HEIGHT);

  // Horizontal scale
  const qreal pix_width = rect().width() - AXIS_LABEL_WIDTH - RIGHT_GAP;
  const qreal x_scale = pix_width / m_scopeView.width();

  // Vertical scale
  const qreal pix_height = rect().height() - AXIS_LABEL_HEIGHT - TOP_GAP;
  const qreal y_scale = pix_height / m_scopeView.height();

  m_modelMatrix.scale(x_scale, y_scale, 1);

  // Translate to current time and vertical offset
  m_modelMatrix.translate(-m_scopeView.left(), -m_scopeView.top(), 0);

  m_mvpMatrix = m_viewMatrix * m_modelMatrix;

  if (m_verticalScaleMode == VerticalScale::Percent)
  {
    // Vertical scale for 16bit
    const qreal y_scale16 = pix_height / (m_scopeView.height() * 256.0);

    QMatrix4x4 modelMatrix16;
    modelMatrix16.translate(AXIS_LABEL_WIDTH, AXIS_LABEL_HEIGHT);
    modelMatrix16.scale(x_scale, y_scale16, 1);

    // Translate to current time and vertical offset
    modelMatrix16.translate(-m_scopeView.left(), -m_scopeView.top(), 0);

    m_mvpMatrix16 = m_viewMatrix * modelMatrix16;
  }
}

std::vector<QVector2D> GlScopeWidget::makeTriggerLine(ScopeModel::Trigger type)
{
  const float level = static_cast<float>(m_model->triggerLevel());
  const float h_offset = m_timeInterval / 6.0;
  float v_offset = 0;

  switch (type)
  {
  default: return std::vector<QVector2D>();

  case ScopeModel::Trigger::Above: v_offset = 10.0f; break;
  case ScopeModel::Trigger::Below: v_offset = -10.0f; break;
  case ScopeModel::Trigger::LevelCross:
    // Two triangles point-to-point
    return {
      { static_cast<float>(m_scopeView.left()) - h_offset, level + 5.0f},
      { static_cast<float>(m_scopeView.left()), level },
      { static_cast<float>(m_scopeView.left()) - h_offset, level - 5.0f },
      { static_cast<float>(m_scopeView.left()) + h_offset, level + 5.0f},
      { static_cast<float>(m_scopeView.left()), level },
      { static_cast<float>(m_scopeView.left()) + h_offset, level - 5.0f }
    };
  }

  return {
    {static_cast<float>(m_scopeView.left()) - h_offset, level},
    {static_cast<float>(m_scopeView.left()) + h_offset , level},
    {static_cast<float>(m_scopeView.left()), level + v_offset}
  };
}

bool ScopeModel::TriggerConfig::isTrigger() const
{
  // Free Run is not a trigger
  if (mode == Trigger::FreeRun)
    return false;

  if (universe > 0 && universe <= MAX_SACN_UNIVERSE && address_hi > 0 && address_hi <= MAX_DMX_ADDRESS
    && address_lo <= MAX_DMX_ADDRESS)
  {
    // Valid coarse byte

    // Check level could trigger
    // 8 or 16bit
    const uint16_t max_level = (address_lo == 0 ? kMaxDmx8 : kMaxDmx16);
    if (level > max_level)
      return false;

    // Can't rise above max
    if (mode == Trigger::Above && level == max_level)
      return false;

    // Can't fall below 0
    if (mode == Trigger::Below && level == 0)
      return false;

    return true;
  }
  return false;
}

bool ScopeModel::TriggerConfig::isTriggerTrace(const ScopeTrace& trace) const
{
  return trace.universe() == universe && trace.addressHi() == address_hi && trace.addressLo() == address_lo;
}

void ScopeModel::TriggerConfig::setTrigger(const ScopeTrace& trace)
{
  universe = trace.universe();
  address_hi = trace.addressHi();
  address_lo = trace.addressLo();
}

QString ScopeModel::TriggerConfig::configurationString() const
{
  if (isTrigger())
  {
    return QStringLiteral("Trigger %1 %2@%3")
      .arg(QMetaEnum::fromType<ScopeModel::Trigger>().valueToKey(static_cast<int>(mode)))
      .arg(ScopeTrace::universeAddressString(universe, address_hi, address_lo))
      .arg(level);
  }
  return QString();
}

void ScopeModel::TriggerConfig::setConfiguration(const QString& configString)
{
  const qsizetype triggerPos = configString.indexOf(QLatin1String("Trigger"));
  if (triggerPos < 0)
  {
    mode = Trigger::FreeRun;
  }
  else
  {
    const auto metaEnum = QMetaEnum::fromType<Trigger>();
    for (int i = 0; i < metaEnum.keyCount(); ++i)
    {
      if (configString.indexOf(QLatin1String(metaEnum.key(i)), triggerPos) > -1)
      {
        mode = static_cast<Trigger>(metaEnum.value(i));
        break;
      }
    }

    // Read trigger value
    const qsizetype triggerUnivPos = configString.indexOf(QLatin1Char(' '), triggerPos + 8);
    const qsizetype triggerLevelPos = configString.indexOf(QLatin1Char('@'), triggerUnivPos);
    uint16_t new_univ, new_addr_hi, new_addr_lo;
    if (ScopeTrace::extractUniverseAddress(configString.mid(triggerUnivPos + 1, triggerLevelPos - triggerUnivPos - 1), new_univ, new_addr_hi, new_addr_lo))
    {
      universe = new_univ;
      address_hi = new_addr_hi;
      address_lo = new_addr_lo;
    }

    const qsizetype triggerLevelEnd = configString.indexOf(QLatin1Char(','), triggerLevelPos) - 1;
    bool ok;
    const qsizetype triggerLevel = configString.mid(triggerLevelPos + 1, triggerLevelEnd - triggerLevelPos).toInt(&ok);
    if (ok)
      level = triggerLevel;
  }
}
