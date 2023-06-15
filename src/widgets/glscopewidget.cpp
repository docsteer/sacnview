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

static const QString RowTitleColor = QStringLiteral("Color");
static const QString ColumnTitleTimestamp = QStringLiteral("Time (s)");

void GlScopeTrace::addPoint(float timestamp, const std::array<int, MAX_DMX_ADDRESS>& level_array)
{
  // Assumes slot numbers are valid
  if (m_slot_lo < MAX_DMX_ADDRESS)
  {
    // 16 bit
    const uint16_t value = level_array[m_slot_hi] << 8 | level_array[m_slot_lo];
    m_trace.emplace_back(timestamp, static_cast<float>(value));
    return;
  }
  // 8 bit
  m_trace.emplace_back(timestamp, static_cast<float>(level_array[m_slot_hi]));
}

void GlScopeTrace::addValue(const QVector2D& value)
{
  m_trace.push_back(value);
}

GlScopeWidget::GlScopeWidget(QWidget* parent)
  : QOpenGLWidget(parent)
{
}

GlScopeWidget::~GlScopeWidget()
{
}

bool GlScopeWidget::addUpdateTrace(const QColor& color, uint16_t universe, uint16_t address_hi, uint16_t address_lo)
{
  // Verify validity
  if (!color.isValid())
    return false;
  if (universe > MAX_SACN_UNIVERSE)
    return false;

  if (address_hi > MAX_DMX_ADDRESS)
    return false;

  if (address_lo > MAX_DMX_ADDRESS)
    address_lo = 0;

  auto univ_it = m_traces.find(universe);
  if (univ_it == m_traces.end())
  {
    m_traces.emplace(universe, std::vector<GlScopeTrace>(1, GlScopeTrace(color, address_hi, address_lo, m_reservation)));

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
    if (item.addressHi() == address_hi && item.addressLo() == address_lo)
    {
      item.setColor(color);
      return true;
    }
  }

  // Add this new trace to the existing universe
  univs_item.emplace_back(color, address_hi, address_lo, m_reservation);
  return true;
}

// Removes all traces that match this
void GlScopeWidget::removeTrace(uint16_t universe, uint16_t address_hi, uint16_t address_lo)
{
  auto univ_it = m_traces.find(universe);
  if (univ_it == m_traces.end())
    return;

  auto& univ_item = univ_it->second;
  for (auto it = univ_item.begin(); it != univ_item.end(); /**/)
  {
    if (it->addressHi() == address_hi && it->addressLo() == address_lo)
    {
      it = univ_item.erase(it);
    }
    else
    {
      ++it;
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

const GlScopeTrace* GlScopeWidget::findTrace(uint16_t universe, uint16_t address_hi, uint16_t address_lo) const
{
  const auto univ_it = m_traces.find(universe);
  if (univ_it != m_traces.end())
  {
    const auto& univ_item = univ_it->second;
    for (auto it = univ_item.begin(); it != univ_item.end(); ++it)
    {
      if (it->addressHi() == address_hi && it->addressLo() == address_lo)
        return &(*it);
    }
  }

  // Not found
  return nullptr;
}

size_t GlScopeWidget::traceCount() const
{
  size_t result = 0;
  for (const auto& univ : m_traces)
  {
    result += univ.second.size();
  }
  return result;
}

void GlScopeWidget::removeAllTraces()
{
  stop();
  m_traces.clear();
}

void GlScopeWidget::clearValues()
{
  // Clear all the trace values
  for (auto& universe : m_traces)
  {
    for (auto& trace : universe.second)
    {
      trace.clear();
    }
  }
}

bool GlScopeWidget::saveTraces(QIODevice& file) const
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
  std::vector<ValueItem> trace_values;

  QString color_header = RowTitleColor;
  QString name_header = ColumnTitleTimestamp;

  // Header rows
  for (const auto& universe : m_traces)
  {
    for (const auto& trace : universe.second)
    {
      // Get the value iterators for this column
      trace_values.emplace_back(trace.values().begin(), trace.values().end());

      // Assemble the color header string
      color_header.append(QLatin1Char(','));
      color_header.append(trace.color().name());

      // Assemble the name header string
      name_header.append(QLatin1String(", U"));
      name_header.append(QString::number(universe.first));
      name_header.append(QLatin1Char('.'));
      name_header.append(QString::number(trace.addressHi()));

      if (trace.isSixteenBit())
      {
        name_header.append(QLatin1Char('/'));
        name_header.append(QString::number(trace.addressLo()));
      }

      // Find the first and last row timestamps
      if (!trace.values().empty())
      {
        if (trace.values().front()[0] < this_row_time)
          this_row_time = trace.values().front()[0];
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

    for (auto& value_its : trace_values)
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

bool GlScopeWidget::loadTraces(QIODevice& file)
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

  while (colors.last().isEmpty())
    colors.pop_back();

  // Remove empty items from the end
  while (titles.last().isEmpty())
    titles.pop_back();

  if (colors.size() != titles.size() || titles.size() < 2)
    return false; // No or invalid data

  // Fairly likely to be valid, clear my data now and stop
  removeAllTraces();

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
    QColor color(colors[i]);

    // Find universe and patch
    const auto& full_title = titles[i];
    const QStringRef title = full_title.trimmed();
    if (title.front() != QLatin1Char('U'))
    {
      trace_idents.push_back(UnivSlots());  // Skip this column
      continue;
    }

    bool ok = false;
    UnivSlots univ_slots;

    // Extract the universe
    const int univ_str_end = title.indexOf(QLatin1Char('.'));
    if (univ_str_end > 0)
    {
      const QStringRef univ_str = title.mid(1, univ_str_end - 1);
      univ_slots.universe = univ_str.toUInt(&ok);
      if (!ok || univ_slots.universe > MAX_SACN_UNIVERSE)
        univ_slots.universe = 0; // Out of bounds
    }

    // Extract slot_hi
    const int slot_str_end = title.indexOf(QLatin1Char('/'));
    {
      const QStringRef slot_hi_str = title.mid(univ_str_end + 1, slot_str_end - univ_str_end - 1);
      univ_slots.address_hi = slot_hi_str.toUInt(&ok);
    }

    // Extract slot_lo
    if (slot_str_end > 0)
    {
      const QStringRef slot_lo_str = title.mid(slot_str_end + 1);
      univ_slots.address_lo = slot_lo_str.toUInt(&ok);
    }

    if (addUpdateTrace(color, univ_slots.universe, univ_slots.address_hi, univ_slots.address_lo))
      trace_idents.push_back(univ_slots);
    else
      trace_idents.push_back(UnivSlots());
  }

  // Now have all the trace idents and the container will not change
  // Get all the pointers and cast away const
  std::vector<GlScopeTrace*> traces;
  for (const auto& ident : trace_idents)
  {
    const GlScopeTrace* trace = findTrace(ident.universe, ident.address_hi, ident.address_lo);
    traces.push_back(const_cast<GlScopeTrace*>(trace));
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
      bool ok = false;
      const float level = data[i].toFloat(&ok);
      if (ok)
        traces[i - 1]->addValue({ timestamp, level });
    }
  }
  return true;
}

bool GlScopeWidget::listeningToUniverse(uint16_t universe) const
{
  return m_traces.count(universe) != 0;
}

void GlScopeWidget::start()
{
  if (isRunning())
    return;

  // Start the clock
  m_elapsed.start();

  for (const auto& universe : m_traces)
  {
    addListener(universe.first);
  }

  emit runningChanged(true);
}

void GlScopeWidget::stop()
{
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

void GlScopeWidget::addListener(uint16_t universe)
{
  auto listener = sACNManager::Instance().getListener(universe);
  connect(listener.data(), &sACNListener::levelsChanged, this, &GlScopeWidget::onLevelsChanged);
  m_listeners.push_back(listener);
}

void GlScopeWidget::onLevelsChanged()
{
  if (!m_elapsed.isValid())
    return;

  sACNListener* listener = qobject_cast<sACNListener*>(sender());

  if (!listener)
    return; // Check for deletion

  // Find traces for universe
  auto it = m_traces.find(listener->universe());
  if (it == m_traces.end())
    return;

  // Grab levels
  const auto levels = listener->mergedLevelsOnly();

  // Time in seconds
  float timestamp = m_elapsed.elapsed();
  timestamp = timestamp / 1000.0f;

  for (GlScopeTrace& trace : it->second)
  {
    trace.addPoint(timestamp, levels);
  }
}

void GlScopeWidget::initializeGL()
{
  // TODO: Implement initializeGL
}

void GlScopeWidget::paintGL()
{
  // TODO: Implement paintGL
}

void GlScopeWidget::resizeGL(int w, int h)
{
  // TODO: Implement resizeGL
}
