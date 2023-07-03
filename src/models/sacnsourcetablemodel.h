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

#pragma once

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

#include "sacn/streamingacn.h"

class SACNSourceTableModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  // The column order in the source table
  enum SC_ROWS
  {
    COL_NAME,
    COL_ONLINE,
    COL_CID,
    COL_UNIVERSE,
    COL_PRIO,
    COL_SYNC,
    COL_PREVIEW,
    COL_IP,
    COL_FPS,
    COL_SEQ_ERR,
    COL_JUMPS,
    COL_VER,
    COL_DD,
    COL_SLOTS,
    COL_PATHWAY_SECURE,
    COL_END
  };

public:
  SACNSourceTableModel(QObject* parent = nullptr);
  ~SACNSourceTableModel();

  int columnCount(const QModelIndex& parent = QModelIndex()) const override { return COL_END; }
  int rowCount(const QModelIndex& parent = QModelIndex()) const override { return static_cast<int>(m_rows.size()); }

  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Add a listener. Does not take ownership
  void addListener(const sACNManager::tListener& listener);
  // Stop updating
  void pause();
  // Restart updates of the existing list of sources and listeners
  void restart();
  // Clear all data and remove all listeners
  void clear();

  // Convenience
  void resetSequenceCounters();
  void resetJumpsCounters();
  void resetCounters();

private Q_SLOTS:
  void sourceOnline(sACNSource* source);
  void sourceChanged(sACNSource* source);

private:
  QHash<sACNSource*, int> m_sourceToTableRow;

  enum class SourceState
  {
    Offline,
    NoDmx,
    Unstable,
    Stable
  };

  enum class SourceSecure
  {
    None,
    BadDigest,
    BadSequence,
    BadPassword,
    Yes
  };

  struct RowData
  {
    RowData() = default;
    RowData(const sACNSource* source);

    QString name;
    CID cid;
    StreamingACNProtocolVersion protocol_version = sACNProtocolUnknown;
    QHostAddress ip;
    float fps = 0;
    unsigned int seq_err = 0;
    unsigned int jumps = 0;
    SourceState online = SourceState::Offline;
    SourceSecure security = SourceSecure::None;
    uint16_t universe = 0;
    uint16_t sync_universe = 0;
    uint16_t slot_count = 0;
    uint8_t priority = 0;
    bool preview = false;
    bool per_address = false;

    void Update(const sACNSource* source);
  };

  std::vector<RowData> m_rows;
  std::vector<sACNManager::wListener> m_listeners;

  // Data
  QVariant getDisplayData(const RowData& rowData, int column) const;
  QVariant getBackgroundData(const RowData& rowData, int column) const;
};
