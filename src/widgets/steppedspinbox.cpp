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

#include "steppedspinbox.h"

SteppedSpinBox::SteppedSpinBox(QWidget* parent)
  : QSpinBox(parent)
{
}

void SteppedSpinBox::setStepList(const QVector<int>& steps, bool setLimits)
{
  m_stepList = steps;
  if (setLimits && !steps.empty())
  {
    setRange(m_stepList.front(), m_stepList.back());
  }
}

void SteppedSpinBox::stepBy(int steps)
{
  if (!m_stepList.empty())
  {
    const int currentValue = value();
    // Step up or down to the next available step
    if (steps > 0)
    {
      // Find first step greater than current value
      auto it = std::upper_bound(m_stepList.begin(), m_stepList.end(), currentValue);
      if (it != m_stepList.end())
      {
        setValue((*it));
        return;
      }
    }
    else if (steps < 0)
    {
      // Find first step greater or equal to current value
      auto it = std::lower_bound(m_stepList.begin(), m_stepList.end(), currentValue);

      if (it != m_stepList.begin())
      {
        // Go back one tick
        --it;
        setValue((*it));
        return;
      }
    }
  }

  // Do the default up/down step
  QSpinBox::stepBy(steps);
}
