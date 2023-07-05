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

#include <QSpinBox>

class SteppedSpinBox : public QSpinBox
{
  Q_OBJECT

public:
  explicit SteppedSpinBox(QWidget* parent = nullptr);

  /**
   * @brief Set the list of step values
   * @param steps List of step values. Must be sorted min to max and contain no duplicates
   * @param setRange True: also set the min & max to the step min/max
  */
  void setStepList(const QVector<int>& steps, bool setRange = true);
  /// Get the list of step values
  const QVector<int>& stepList() const { return m_stepList; }

  void stepBy(int steps) override;

protected:
  QVector<int> m_stepList;
};