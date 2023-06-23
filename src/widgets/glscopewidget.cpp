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

static constexpr qreal AXIS_LABEL_WIDTH = 50.0;
static constexpr qreal AXIS_LABEL_HEIGHT = 20.0;
static constexpr qreal TOP_GAP = 10.0;
static constexpr qreal RIGHT_GAP = 15.0;
static constexpr qreal AXIS_TO_WINDOW_GAP = 5.0;
static constexpr qreal AXIS_TICK_SIZE = 10.0;

static const QString RowTitleColor = QStringLiteral("Color");
static const QString ColumnTitleTimestamp = QStringLiteral("Time (s)");

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

QString ScopeTrace::universeAddressString() const
{
  return QStringLiteral("U") + QString::number(universe()) + QLatin1Char('.') + addressString();
}

QString ScopeTrace::addressString() const
{
  return isSixteenBit() ?
    QString::number(addressHi()) + QLatin1Char('/') + QString::number(addressLo())
    : QString::number(addressHi());
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
inline bool fillValue(T& value, uint16_t slot_hi, uint16_t slot_lo, const std::array<int, MAX_DMX_ADDRESS>& level_array)
{
  // Assumes slot numbers are valid
  if (slot_lo < MAX_DMX_ADDRESS)
  {
    // 16 bit
    // Do nothing if no level yet
    if (level_array[slot_hi] < 0 || level_array[slot_lo] < 0)
      return false;

    value = static_cast<uint16_t>(level_array[slot_hi] << 8) | static_cast<uint16_t>(level_array[slot_lo]);
    return true;
  }
  // 8 bit
  // Do nothing if no level
  if (level_array[slot_hi] < 0)
    return false;

  value = static_cast<T>(level_array[slot_hi]);
  return true;
}

void ScopeTrace::addPoint(float timestamp, const std::array<int, MAX_DMX_ADDRESS>& level_array)
{
  float value;
  if (fillValue(value, m_slot_hi, m_slot_lo, level_array))
    m_trace.emplace_back(timestamp, value);
}

void ScopeTrace::setFirstPoint(const std::array<int, MAX_DMX_ADDRESS>& level_array)
{
  float value;
  if (fillValue(value, m_slot_hi, m_slot_lo, level_array))
  {
    if (m_trace.empty())
      m_trace.emplace_back(0, value);
    else
      m_trace[0].setY(value);
  }
}

bool ScopeModel::addUpdateTrace(const QColor& color, uint16_t universe, uint16_t address_hi, uint16_t address_lo)
{
  // Verify validity
  if (!color.isValid())
    return false;

  if (universe == 0)
    return false;

  if (address_hi == 0)
    return false;

  if (universe > MAX_SACN_UNIVERSE)
    return false;

  if (address_hi > MAX_DMX_ADDRESS)
    return false;

  if (address_lo > MAX_DMX_ADDRESS)
    address_lo = 0;
  else if (address_lo > 0)
  {
    // 16bit, adjust vertical scale
    m_maxValue = 65535;
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

    return true;
  }

  // If we have already got this trace, update the color
  auto& univs_item = univ_it->second;
  for (auto& item : univs_item)
  {
    if (item->addressHi() == address_hi && item->addressLo() == address_lo)
    {
      item->setColor(color);
      return true;
    }
  }

  // Add this new trace to the existing universe
  beginInsertRows(QModelIndex(), m_traceTable.size(), m_traceTable.size());
  ScopeTrace* trace = new ScopeTrace(color, universe, address_hi, address_lo, m_reservation);
  m_traceTable.push_back(trace);
  univs_item.push_back(trace);
  endInsertRows();
  return true;
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
        disconnect((*it).data(), nullptr, this, nullptr);
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

const ScopeTrace* ScopeModel::findTrace(uint16_t universe, uint16_t address_hi, uint16_t address_lo) const
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

ScopeModel::ScopeModel(QObject* parent)
  : QAbstractTableModel(parent)
{
  private_removeAllTraces();
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
        break;
      case COL_ADDRESS:
        if (role == Qt::DisplayRole || role == Qt::EditRole) return trace->addressString();
        if (role == DataSortRole) return uint32_t(trace->addressHi()) << 16 | trace->addressLo();
        break;
      case COL_COLOUR:
        if (role == Qt::BackgroundRole || role == Qt::DisplayRole || role == Qt::EditRole) return trace->color();
        if (role == DataSortRole) return static_cast<uint32_t>(trace->color().rgba());
        break;
      case COL_TRIGGER:
        if (role == Qt::CheckStateRole)
          return m_trigger.IsTriggerTrace(*trace) ? Qt::Checked : Qt::Unchecked;
        if (role == DataSortRole) return m_trigger.IsTriggerTrace(*trace) ? 0 : 1;
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

  if (idx.column() < COL_COUNT && idx.row() < rowCount())
  {
    ScopeTrace* trace = m_traceTable.at(idx.row());
    if (trace)
    {
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
          if (moveTrace(trace, value.toUInt()))
          {
            emit dataChanged(idx, idx, { Qt::DisplayRole, Qt::EditRole, DataSortRole });
            return true;
          }
        }
        break;

      case COL_ADDRESS:
        if (role == Qt::EditRole)
        {
          if (trace->setAddress(value.toString(), true))
          {
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
            m_trigger.universe = trace->universe();
            m_trigger.address_hi = trace->addressHi();
            m_trigger.address_lo = trace->addressLo();
            emit dataChanged(index(0, COL_TRIGGER), index(rowCount() - 1, COL_TRIGGER), { Qt::CheckStateRole, DataSortRole });
            return true;
          }
        }
        break;
      }
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
  for (ScopeTrace* trace : m_traceTable)
    delete trace;

  m_traceLookup.clear();
  m_traceTable.clear();
  // Set extents back to default
  m_endTime = 0;
  m_maxValue = 255;
}

void ScopeModel::clearValues()
{
  // Clear all the trace values
  for (ScopeTrace* trace : m_traceTable)
  {
    trace->clear();
  }

  // Reset time extents
  m_endTime = 0;
}

bool ScopeModel::saveTraces(QIODevice& file) const
{
  if (!file.isWritable())
    return false;

  // Cannot write while running
  if (isRunning())
    return false;

  QTextStream out(&file);
  out.setCodec("UTF-8");
  out.setLocale(QLocale::c());
  out.setRealNumberNotation(QTextStream::FixedNotation);
  out.setRealNumberPrecision(3);

  // Table:
  // Color,     red, green, ...
  // Time (s), U1.1, U1.2/3, ... (Given as Universe.CoarseDMX/FineDmx (1-512)
  // 0.000,     255,    0, ...
  // 0.020,     128,  128, ...
  // 0.040,     127,  255, ...

  // First row time
  float this_row_time = std::numeric_limits<float>::max();

  // Iterators for each trace
  using ValueIterator = std::vector<QVector2D>::const_iterator;
  struct ValueItem
  {
    ValueItem(ValueIterator c, ValueIterator e) : current(c), end(e) {}
    ValueIterator current;
    ValueIterator end;
  };
  std::vector<ValueItem> traces_values;

  QString color_header = RowTitleColor;
  QString name_header = ColumnTitleTimestamp;

  // Header rows
  for (const auto& universe : m_traceLookup)
  {
    for (const ScopeTrace* trace : universe.second)
    {
      // Get the value iterators for this column
      traces_values.emplace_back(trace->values().begin(), trace->values().end());

      // Assemble the color header string
      color_header.append(QLatin1Char(','));
      color_header.append(trace->color().name());

      // Assemble the name header string
      name_header.append(QLatin1String(", "));
      name_header.append(trace->universeAddressString());

      // Find the first and last row timestamps
      if (!trace->values().empty())
      {
        if (trace->values().front()[0] < this_row_time)
          this_row_time = trace->values().front()[0];
      }
    }
  }
  out << color_header << '\n' << name_header;

  // Value rows
  while (this_row_time < std::numeric_limits<float>::max())
  {
    // Start new row and output timestamp
    out << '\n' << this_row_time;
    float next_row_time = std::numeric_limits<float>::max();

    for (auto& value_its : traces_values)
    {
      // Next Column
      out << ',';
      if (value_its.current != value_its.end)
      {
        // This column has a value for this time
        if (qFuzzyCompare(this_row_time, (*value_its.current)[0]))
        {
          // Output a value for this timestamp and step forward
          out << static_cast<int>((*value_its.current)[1]);
          ++value_its.current;

          // Determine the next row time
          if (value_its.current != value_its.end && (*value_its.current)[0] < next_row_time)
          {
            next_row_time = (*value_its.current)[0];
          }
        }
      }
    }
    // On to the next row
    this_row_time = next_row_time;
  }

  return true;
}

QPair<QString, QString> FindUniverseTitles(QTextStream& in)
{
  // Find start of data
  QString top_line;
  while (in.readLineInto(&top_line))
  {
    if (top_line.startsWith(RowTitleColor))
    {
      // Probably the title line, remove the timestamp title
      top_line.remove(0, RowTitleColor.size() + 1);
      QString next_line = in.readLine();
      if (!next_line.startsWith(ColumnTitleTimestamp))
        return QPair<QString, QString>();// Failed

      next_line.remove(0, ColumnTitleTimestamp.size() + 1);
      return { top_line, next_line };
    }
  }
  return QPair<QString, QString>();
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
  if (title_line.second.isEmpty())
    return false;

  // Split the title lines to find the trace colors and names
  auto colors = title_line.first.splitRef(QLatin1Char(','), Qt::KeepEmptyParts);
  auto titles = title_line.second.splitRef(QLatin1Char(','), Qt::KeepEmptyParts);

  // Remove empty colors from the end
  while (colors.last().isEmpty())
    colors.pop_back();
  // Remove empty items from the end
  while (titles.last().isEmpty())
    titles.pop_back();

  if (colors.size() != titles.size() || titles.size() < 2)
    return false; // No or invalid data

  // Fairly likely to be valid, stop and clear my data now
  beginResetModel();
  private_removeAllTraces();

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

    if (addUpdateTrace(color, univ_slots.universe, univ_slots.address_hi, univ_slots.address_lo))
      trace_idents.push_back(univ_slots);
    else
      trace_idents.push_back(UnivSlots());
  }

  // Now have all the trace idents and the container will not change
  // Get all the pointers and cast away const
  std::vector<ScopeTrace*> traces;
  for (const auto& ident : trace_idents)
  {
    const ScopeTrace* trace = findTrace(ident.universe, ident.address_hi, ident.address_lo);
    traces.push_back(const_cast<ScopeTrace*>(trace));
  }

  QString data_line;
  float prev_timestamp = 0;
  while (in.readLineInto(&data_line))
  {
    if (data_line.isEmpty())
      continue;

    const auto data = data_line.splitRef(QLatin1Char(','), Qt::KeepEmptyParts);
    // Ignore any lines that do not have a column for all traces
    if (data.size() < traces.size() + 1)
      continue;

    // Time moves ever forward. Ignore any lines in the past
    bool ok = false;
    const float timestamp = data[0].toFloat(&ok);
    if (!ok || prev_timestamp > timestamp)
      continue;

    prev_timestamp = timestamp;

    for (size_t i = 1; i <= traces.size(); ++i)
    {
      ScopeTrace* trace = traces[i - 1];
      if (trace)
      {
        bool ok = false;
        const float level = data[i].toFloat(&ok);
        if (ok)
          trace->addValue({ timestamp, level });
      }
    }
  }

  m_endTime = prev_timestamp;

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
  if (m_trigger.IsTrigger())
  {
    m_trigger.last_level = -1;
  }
  else
  {
    m_elapsed.start();
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

  // Disconnect all
  for (auto& listener : m_listeners)
  {
    disconnect(listener.data(), nullptr, this, nullptr);
  }
  // And clear/shutdown
  m_listeners.clear();
  m_elapsed.invalidate();
  emit runningChanged(false);
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

void ScopeModel::setTriggerDelay(qint64 millisecs)
{
  m_trigger.delay = millisecs;
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
          disconnect((*it).data(), nullptr, this, nullptr);
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
  connect(listener.data(), &sACNListener::dmxReceived, this, &ScopeModel::onDmxReceived);
  readLevels(listener.data());
  m_listeners.push_back(listener);
}

void ScopeModel::triggerNow()
{
  m_elapsed.start();
  emit triggered();
}

QRectF ScopeModel::traceExtents() const
{
  return QRectF(0, 0, m_endTime, m_maxValue);
}

qreal ScopeModel::endTime() const
{
  if (isTriggered())
    return qreal(m_elapsed.elapsed()) / 1000;

  return m_endTime;
}

void ScopeModel::onDmxReceived()
{
  if (!isRunning())
    return;

  sACNListener* listener = qobject_cast<sACNListener*>(sender());

  if (!listener)
    return; // Check for deletion

  readLevels(listener);
}

void ScopeModel::readLevels(sACNListener* listener)
{
  // Find traces for universe
  auto it = m_traceLookup.find(listener->universe());
  if (it == m_traceLookup.end())
    return;

  // Grab levels
  const auto levels = listener->mergedLevelsOnly();

  if (!isTriggered())
  {
    {
      uint16_t current_level;
      if (listener->universe() == m_trigger.universe && fillValue(current_level, m_trigger.address_hi - 1, m_trigger.address_lo - 1, levels))
      {
        // Have a current level, maybe start the clock
        switch (m_trigger.mode)
        {
        default: break;
        case Trigger::Above:
          if (current_level > m_trigger.level)
            triggerNow();
          break;
        case Trigger::Below:
          if (current_level < m_trigger.level)
            triggerNow();
          break;
        case Trigger::LevelCross:
          if ((m_trigger.last_level == m_trigger.level && current_level != m_trigger.level) // Was at, now not
            || (m_trigger.last_level > m_trigger.level && current_level < m_trigger.level) // Was above, now below
            || (m_trigger.last_level != -1 && m_trigger.last_level < m_trigger.level && current_level > m_trigger.level) // Was below, now above
            )
            triggerNow();
          break;
        }
        m_trigger.last_level = current_level;
      }
    }

    // Only store the zero-time level for each trace until the trigger is fired
    // Store the values for zero time
    for (ScopeTrace* trace : it->second)
    {
      trace->setFirstPoint(levels);
    }
    return;
  }

  // Time in seconds
  {
    const qreal timestamp = m_elapsed.elapsed();
    m_endTime = timestamp / 1000.0;
  }

  for (ScopeTrace* trace : it->second)
  {
    trace->addPoint(m_endTime, levels);
  }
}

GlScopeWidget::GlScopeWidget(QWidget* parent)
  : QOpenGLWidget(parent)
{
  setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

  m_model = new ScopeModel(this);
  connect(m_model, &ScopeModel::runningChanged, this, &GlScopeWidget::onRunningChanged);
  connect(m_model, &ScopeModel::traceVisibilityChanged, this, QOverload<void>::of(&QOpenGLWidget::update));

  setMinimumSize(200, 200);
  setVerticalScaleMode(VerticalScale::Dmx);
  setScopeView();
}

GlScopeWidget::~GlScopeWidget()
{
  cleanupGL();
}

void GlScopeWidget::setVerticalScaleMode(VerticalScale scaleMode)
{
  if (scaleMode == m_verticalScaleMode)
    return;

  switch (scaleMode)
  {
  default:
    return; // Invalid, do nothing
  case VerticalScale::Percent:
  {
    m_levelInterval = 10;
  } break;
  case VerticalScale::Dmx:
  {
    m_levelInterval = 20;
  } break;
  }

  m_verticalScaleMode = scaleMode;

  update();
}

void GlScopeWidget::setScopeView(const QRectF& rect)
{
  if (rect.isEmpty())
  {
    m_scopeView = m_model->traceExtents();
    m_scopeView.setRight(m_defaultIntervalCount * m_timeInterval);
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
    "void main()\n"
    "{\n"
    "  gl_Position = mvp * vec4(vertex.x, vertex.y, 0, 1.0);\n"
    "}\n";

  const char* fragmentShaderSource =
    "uniform vec4 color;\n"
    "void main()\n"
    "{\n"
    "  gl_FragColor = color;\n"
    "}\n";

  m_program = new QOpenGLShaderProgram(this);
  if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource))
  {
    qDebug() << "Vertex Shader Failed:" << m_program->log();
    cleanupGL();
    return;
  }

  if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource))
  {
    qDebug() << "Fragment Shader Failed:" << m_program->log();
    cleanupGL();
    return;
  }

  if (!m_program->link())
  {
    qDebug() << "Shader Link Failed:" << m_program->log();
    cleanupGL();
    return;
  }

  m_vertexLocation = m_program->attributeLocation("vertex");
  Q_ASSERT(m_vertexLocation != -1);
  m_matrixUniform = m_program->uniformLocation("mvp");
  Q_ASSERT(m_matrixUniform != -1);
  m_colorUniform = m_program->uniformLocation("color");
  Q_ASSERT(m_colorUniform != -1);
}

void GlScopeWidget::cleanupGL()
{
  // The context is about to be destroyed, it may be recreated later
  if (m_program == nullptr)
    return; // Never started

  makeCurrent();

  delete m_program;
  m_program = nullptr;

  doneCurrent();
}

inline void DrawLevelAxisMark(QPainter& painter, const QFontMetricsF& metrics,
  const QPen& gridPen, const QPen& textPen,
  const QRectF& scopeWindow, int level, qreal y_scale, const QString& postfix)
{
  qreal y = scopeWindow.height() - (level * y_scale);
  painter.setPen(gridPen);
  painter.drawLine(QPointF(-AXIS_TICK_SIZE, y), QPointF(scopeWindow.width(), y));

  // TODO: use QStaticText to optimise the text layout
  const QString text = QString::number(level) + postfix;

  QRectF fontRect = metrics.boundingRect(text);
  fontRect.moveBottomRight(QPointF(-AXIS_TICK_SIZE, y + (fontRect.height() / 2.0)));

  painter.setPen(textPen);
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

  // Draw the axes using QPainter as is easiest way to get the text
  {
    QPen gridPen;
    gridPen.setColor(QColor("#434343"));

    QPen textPen;
    textPen.setColor(Qt::white);

    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Origin is the bottom left of the scope window
    QPointF origin(rect().bottomLeft().x() + AXIS_LABEL_WIDTH, rect().bottomLeft().y() - AXIS_LABEL_HEIGHT);

    QRectF scopeWindow;
    scopeWindow.setBottomLeft(origin);
    scopeWindow.setTop(rect().top() + TOP_GAP);
    scopeWindow.setRight(rect().right() - RIGHT_GAP);

    // Draw nothing if tiny
    if (scopeWindow.width() < AXIS_TICK_SIZE)
      return;

    QFont font;
    QFontMetricsF metrics(font);

    painter.translate(scopeWindow.topLeft().x(), scopeWindow.topLeft().y());

    // Draw vertical (level) axis
    {
      painter.setPen(gridPen);
      painter.drawLine(QPointF(0, 0), QPointF(0, scopeWindow.height()));

      QString postfix;
      int max_value = m_scopeView.bottom();
      qreal y_scale = scopeWindow.height() / m_scopeView.bottom();

      if (m_verticalScaleMode == VerticalScale::Percent)
      {
        postfix = QStringLiteral("%");
        // TODO: Scale this properly
        max_value = 100;
        y_scale = scopeWindow.height() / 100.0;
      }

      // From bottom to top
      for (int value = m_scopeView.top(); value < max_value; value += m_levelInterval)
      {
        DrawLevelAxisMark(painter, metrics, gridPen, textPen, scopeWindow, value, y_scale, postfix);
      }

      // Final row, may be uneven
      DrawLevelAxisMark(painter, metrics, gridPen, textPen, scopeWindow, max_value, y_scale, postfix);
    }

    // Draw horizontal (time) axis
    painter.resetTransform();
    painter.translate(scopeWindow.bottomLeft().x(), scopeWindow.bottomLeft().y());

    const qreal x_scale = scopeWindow.width() / m_scopeView.width();
    const bool milliseconds = (m_timeInterval < 1.0);

    for (qreal time = roundCeilMultiple(m_scopeView.left(), m_timeInterval); time < m_scopeView.right() + 0.001; time += m_timeInterval)
    {
      const qreal x = (time - m_scopeView.left()) * x_scale;
      painter.setPen(gridPen);
      painter.drawLine(x, 0, x, -scopeWindow.height());

      // TODO: use QStaticText to optimise the text layout
      const QString text = milliseconds ? QStringLiteral("%1ms").arg(time * 1000.0) : QStringLiteral("%1s").arg(time);

      QRectF fontRect = metrics.boundingRect(text);

      fontRect.moveCenter(QPointF(x, AXIS_LABEL_HEIGHT / 2.0));

      painter.setPen(textPen);
      painter.drawText(fontRect, text, QTextOption(Qt::AlignLeft));
    }
  }

  // TODO: Draw the graph lines using the mvpMatrix to ensure they match and improve performance
  // It's ok if the labels aren't pixel perfect

  // Unable to render at all
  if (!m_program)
    return;

  m_program->bind();

  m_program->setUniformValue(m_matrixUniform, m_mvpMatrix);

  if (m_model->isTriggered())
  {
    // Draw the current time
    m_program->setUniformValue(m_colorUniform, QColor(Qt::white));
    const std::vector<QVector2D> nowLine = {
      {static_cast<float>(m_model->endTime()), 0},
      {static_cast<float>(m_model->endTime()), 65535}
    };
    glVertexAttribPointer(m_vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, nowLine.data());

    glEnableVertexAttribArray(m_vertexLocation);

    glDrawArrays(GL_LINE_STRIP, 0, 2);

    glDisableVertexAttribArray(m_vertexLocation);
  }
  else if (m_model->triggerType() != ScopeModel::Trigger::FreeRun)
  {
    // Draw the trigger level as a triangle pointing up or down
    m_program->setUniformValue(m_colorUniform, QColor(Qt::gray));

    const std::vector<QVector2D> triggerLine = makeTriggerLine(m_model->triggerType());

    glVertexAttribPointer(m_vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, triggerLine.data());

    glEnableVertexAttribArray(m_vertexLocation);

    glDrawArrays(GL_TRIANGLES, 0, triggerLine.size());

    glDisableVertexAttribArray(m_vertexLocation);
  }


  for (ScopeTrace* trace : m_model->traces())
  {
    if (!trace->enabled())
      continue;

    m_program->setUniformValue(m_colorUniform, trace->color());

    glVertexAttribPointer(m_vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, trace->values().data());

    glEnableVertexAttribArray(m_vertexLocation);

    glDrawArrays(GL_LINE_STRIP, 0, trace->values().size());

    glDisableVertexAttribArray(m_vertexLocation);
  }

  m_program->release();

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

void GlScopeWidget::onRunningChanged(bool running)
{
  if (running)
  {
    if (m_renderTimer == 0)
    {
      // Redraw at half the screen refresh or 5fps, whichever is greater
      const int framerate = screen()->refreshRate() / 2;
      m_renderTimer = startTimer(1 / (framerate > 5 ? framerate : 5));
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
  QMatrix4x4 modelMatrix;

  // Translate to origin
  modelMatrix.translate(AXIS_LABEL_WIDTH, AXIS_LABEL_HEIGHT);

  // Vertical scale
  const qreal pix_height = rect().height() - AXIS_LABEL_HEIGHT - TOP_GAP;
  const qreal y_scale = pix_height / m_scopeView.height();

  // Horizontal scale
  const qreal pix_width = rect().width() - AXIS_LABEL_WIDTH - RIGHT_GAP;
  const qreal x_scale = pix_width / m_scopeView.width();

  modelMatrix.scale(x_scale, y_scale, 1);

  // Translate to current time and vertical offset
  modelMatrix.translate(-m_scopeView.left(), -m_scopeView.top(), 0);

  m_mvpMatrix = m_viewMatrix * modelMatrix;
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

bool ScopeModel::TriggerConfig::IsTrigger() const
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
    const uint16_t max_level = (address_lo == 0 ? 255 : 65535);
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

bool ScopeModel::TriggerConfig::IsTriggerTrace(const ScopeTrace& trace) const
{
  return trace.universe() == universe && trace.addressHi() == address_hi && trace.addressLo() == address_lo;
}
