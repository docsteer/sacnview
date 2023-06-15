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

#pragma once

#include <QOpenGLWidget>
#include <QVector2D>
#include <QElapsedTimer>

#include "sacn/sacnlistener.h"

#include <optional>

class GlScopeTrace
{
public:
  /**
   * @brief
   * @param color The color to render the trace
   * @param address_hi The DMX address (1-512) of the Coarse byte
   * @param address_lo The DMX address (1-512) of the Fine byte
   * @param reservation How many points to reserve ahead of time
  */
  GlScopeTrace(const QColor& color, uint16_t address_hi, uint16_t address_lo, size_t reservation)
    : m_color(color)
    , m_slot_hi(address_hi - 1)
    , m_slot_lo(address_lo - 1)
  {
    reserve(reservation);
  }

  // Immutable
  uint16_t addressHi() const { return m_slot_hi + 1; }
  uint16_t addressLo() const { return m_slot_lo + 1; }
  bool isSixteenBit() const { return m_slot_lo < MAX_DMX_ADDRESS; }
  bool isValid() const { return m_color.isValid() && m_slot_hi < MAX_DMX_ADDRESS; }

  bool enabled() const { return m_enabled; };
  void setEnabled(bool value) { m_enabled = value; };

  const QColor& color() const { return m_color; };
  void setColor(const QColor& color) { m_color = color; };

  void clear() { m_trace.clear(); }
  void reserve(size_t point_count) { m_trace.reserve(point_count); }

  void addPoint(float timestamp, const std::array<int, MAX_DMX_ADDRESS>& level_array);

  const std::vector<QVector2D>& values() const { return m_trace; }

  // For loading from CSV
  void addValue(const QVector2D& value);

private:
  QColor m_color;
  std::vector<QVector2D> m_trace;
  uint16_t m_slot_hi = 0;
  uint16_t m_slot_lo = 0xFFFF;
  bool m_enabled = true;
};

class GlScopeWidget : public QOpenGLWidget
{
  Q_OBJECT
public:
  explicit GlScopeWidget(QWidget* parent = nullptr);
  ~GlScopeWidget();

  /**
   * @brief Add or update a trace
   * @param color Color to use for the trace. Must be valid
   * @param universe Universe for the trace. (1 - Max sACN Universe)
   * @param address_hi DMX address of the Coarse byte. (1-512)
   * @param address_lo DMX address of the Fine byte. Out of range for 8bit.
   * @return true if added or updated the trace, false for invalid parameters
  */
  bool addUpdateTrace(const QColor& color, uint16_t universe, uint16_t address_hi, uint16_t address_lo = 0);

  /**
   * @brief Remove a trace
   * @param universe Universe
   * @param address_hi DMX address of the Coarse byte. (1-512)
   * @param address_lo DMX address of the Fine byte. Out of range for 8bit.
  */
  void removeTrace(uint16_t universe, uint16_t address_hi, uint16_t address_lo = 0);

  /**
   * @brief Find the scope trace object for a universe and slot pair
   * Caution: This pointer is invalidated if any traces are added or removed
   * @param universe Universe
   * @param address_hi DMX address of the Coarse byte. (1-512)
   * @param address_lo DMX address of the Fine byte. Out of range for 8bit. (Valid range 1-512)
   * @return
  */
  const GlScopeTrace* findTrace(uint16_t universe, uint16_t address_hi, uint16_t address_lo = 0) const;

  /// Get count of registered traces
  size_t traceCount() const;

  /**
   * @brief Stop and remove all traces
  */
  void removeAllTraces();

  /**
   * @brief Clear all trace values but leave the traces ready for another run
  */
  void clearValues();

  /**
   * @brief Store the traces as a CSV file segment. Scope must be stopped.
   * @param file IODevice to store to. Must be open for writing.
   * @return succeeded
  */
  bool saveTraces(QIODevice& file) const;

  /**
   * @brief Load traces from a CSV file segment. Scope will stop.
   * @param file IODevice to load from. Must be open for reading.
   * @return succeeded
  */
  bool loadTraces(QIODevice& file);

  /**
   * @brief Check if already listening to a universe
   * @param universe
   * @return true if listening
  */
  bool listeningToUniverse(uint16_t universe) const;

  /**
   * @brief Start listening and storing trace info
  */
  Q_SLOT void start();
  /**
   * @brief Stop adding data to the traces
  */
  Q_SLOT void stop();
  /// @return true if currently running
  bool isRunning() const { return m_elapsed.isValid(); }
  Q_SIGNAL void runningChanged(bool running);

  // TODO: Triggers

private:
  Q_SLOT void onLevelsChanged();

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

private:
  std::map<uint16_t, std::vector<GlScopeTrace>> m_traces;
  std::vector<sACNManager::tListener> m_listeners; // Keep the listeners alive
  QElapsedTimer m_elapsed;
  size_t m_reservation = 10000; // Reserve space for this many samples

  void addListener(uint16_t universe);
};