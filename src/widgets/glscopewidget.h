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
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector2D>
#include <QAbstractTableModel>
#include <QElapsedTimer>

#include "sacn/sacnlistener.h"

class ScopeTrace
{
public:
  /**
   * @brief
   * @param color The color to render the trace
   * @param universe The sACN universe number
   * @param address_hi The DMX address (1-512) of the Coarse byte
   * @param address_lo The DMX address (1-512) of the Fine byte
   * @param reservation How many points to reserve ahead of time
  */
  ScopeTrace(const QColor& color, uint16_t universe, uint16_t address_hi, uint16_t address_lo, size_t reservation)
    : m_color(color)
    , m_universe(universe)
    , m_slot_hi(address_hi - 1)
    , m_slot_lo(address_lo - 1)
  {
    reserve(reservation);
  }

  // String conversion
  static bool extractUniverseAddress(QStringView address_string, uint16_t& universe, uint16_t& address_hi, uint16_t& address_lo);
  static bool extractAddress(QStringView address_string, uint16_t& address_hi, uint16_t& address_lo);

  QString universeAddressString() const;
  QString addressString() const;

  uint16_t universe() const { return m_universe; }
  bool setUniverse(uint16_t new_universe, bool clear_values = true);

  uint16_t addressHi() const { return m_slot_hi + 1; }
  uint16_t addressLo() const { return m_slot_lo + 1; }
  bool setAddress(uint16_t address_hi, uint16_t address_lo, bool clear_values = true);
  bool setAddress(QStringView addressString, bool clear_values = true);
  bool isSixteenBit() const { return m_slot_lo < MAX_DMX_ADDRESS; }

  bool isValid() const { return m_color.isValid() && m_universe != 0 && m_slot_hi < MAX_DMX_ADDRESS; }

  bool enabled() const { return m_enabled; };
  void setEnabled(bool value) { m_enabled = value; };

  const QColor& color() const { return m_color; };
  void setColor(const QColor& color) { m_color = color; };

  void clear() { m_trace.clear(); }
  void reserve(size_t point_count) { m_trace.reserve(point_count); }

  void addPoint(float timestamp, const std::array<int, MAX_DMX_ADDRESS>& level_array);

  const std::vector<QVector2D>& values() const { return m_trace; }

  // For loading from CSV
  void addValue(const QVector2D& value) { m_trace.push_back(value); }

private:
  std::vector<QVector2D> m_trace;
  QColor m_color;
  uint16_t m_universe = 0;
  uint16_t m_slot_hi = 0;
  uint16_t m_slot_lo = 0xFFFF;
  bool m_enabled = true;
};

class ScopeModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  enum Columns
  {
    COL_UNIVERSE,
    COL_ADDRESS,
    COL_COLOUR,
    COL_TRIGGER,
    COL_COUNT
  };

  enum class Trigger
  {
    FreeRun,
    RisingEdge,
    FallingEdge
  };

public:
  ScopeModel(QObject* parent = nullptr);
  ~ScopeModel();

  /// QAbstractTableModel interface
  int columnCount(const QModelIndex& parent = QModelIndex()) const override { return COL_COUNT; }
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  Qt::ItemFlags flags(const QModelIndex& index = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

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
  const ScopeTrace* findTrace(uint16_t universe, uint16_t address_hi, uint16_t address_lo = 0) const;

  /**
   * @brief Get all traces for rendering
  */
  const std::vector<ScopeTrace*>& traces() const { return m_traceTable; }

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

  // Triggers
  void setTriggerType(Trigger mode) { m_trigger.mode = mode; }
  Trigger triggerType() const { return m_trigger.mode; }

  Q_SLOT void setTriggerLevel(uint16_t level) { m_trigger.level = level; }
  uint16_t triggerLevel() const { return m_trigger.level; }

  Q_SLOT void setTriggerDelay(qint64 millisecs) { m_trigger.delay = millisecs; }
  qint64 getTriggerDelay() const { return m_trigger.delay; }

  /**
   * @brief Get current overall trace extents
   * Suitable to use as zoom-to-extents
  */
  QRectF traceExtents() const { return m_traceExtents; }

private:
  Q_SLOT void onLevelsChanged();

private:
  std::vector<ScopeTrace*> m_traceTable;
  std::map<uint16_t, std::vector<ScopeTrace*>> m_traceLookup;
  std::vector<sACNManager::tListener> m_listeners; // Keep the listeners alive
  QElapsedTimer m_elapsed;
  QRectF m_traceExtents; // Maximum extents of the scope measurements in DMX

  struct
  {
    uint16_t universe = 0;
    uint16_t address_hi = 0;
    uint16_t address_lo = 0;
    uint16_t level = 0;
    qint64 delay = 0;
    Trigger mode = Trigger::FreeRun;
    bool triggered = false;

    bool IsTriggerTrace(const ScopeTrace& trace) const
    {
      return trace.universe() == universe && trace.addressHi() == address_hi && trace.addressLo() == address_lo;
    }
  } m_trigger; // Trigger configuration

  size_t m_reservation = 12000; // Reserve space for this many samples. 300s @ 40Hz

  void private_removeAllTraces();
  // Move a trace from one universe to another if possible
  bool moveTrace(ScopeTrace* trace, uint16_t new_universe, bool clear_values = true);
  void addListener(uint16_t universe);
};

class GlScopeWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT
public:
  enum class VerticalScale
  {
    Invalid,
    Dmx,
    Percent,
  };

public:
  explicit GlScopeWidget(QWidget* parent = nullptr);
  ~GlScopeWidget();

  QSize minimumSizeHint() const override { return QSize(512, 256); }

  ScopeModel* model() { return m_model; }
  const ScopeModel* model() const { return m_model; }

  VerticalScale verticalScaleMode() const { return m_verticalScaleMode; }
  void setVerticalScaleMode(VerticalScale scaleMode);

  /**
   * @brief Get the current scope view
   * x is time axis in seconds (0 to ...)
   * y is scale axis in raw DMX values (0-255 or 65535)
   * Note: Y axis is flipped compared to Qt: (0,0) is bottom-left.
   * QRectF::top() is the bottom
   * QRectF::bottom() is the top
   * @return current viewport
  */
  const QRectF& scopeView() const { return m_scopeView; }

  /**
   * @brief Set the scope view
   * @param rect new range rectangle. Null to reset to default extents
  */
  Q_SLOT void setScopeView(const QRectF& rect = QRectF());

  int timeDivisions() const { return m_timeInterval * 1000.0; }
  Q_SLOT void setTimeDivisions(int milliseconds);
  Q_SIGNAL void timeDivisionsChanged(int milliseconds);

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

private:
  ScopeModel* m_model = nullptr;

  // View configuration
  VerticalScale m_verticalScaleMode = VerticalScale::Invalid;
  int m_levelInterval = 20; // Level axis label interval
  qreal m_defaultViewWidth = 10.0; // Seconds to show when reset to default
  qreal m_timeInterval = 1.0; // Time axis label interval

  QRectF m_scopeView; // Current scope view range in DMX

  // Rendering configuration
  QMatrix4x4 m_viewMatrix;
  QMatrix4x4 m_mvpMatrix;

  void updateMVPMatrix();
};