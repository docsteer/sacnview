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

#include "gtest/gtest.h"

#include "widgets/glscopewidget.h"

#include <QBuffer>
#include <QFile>

TEST(ScopeTrace, AddPoint)
{
  // Storing all points, even if same level

  ScopeTrace trace(Qt::red, 1, 1, 0, 10);
  EXPECT_TRUE(trace.isValid());
  EXPECT_FALSE(trace.isSixteenBit());
  EXPECT_TRUE(trace.values().value().empty());

  const std::array<int, MAX_DMX_ADDRESS> levels = { 1, 2 };
  float time = 0;
  trace.addPoint(time, levels, true);
  ASSERT_EQ(1, trace.values().value().size());
  EXPECT_EQ(QVector2D(time, 1), trace.values().value().back());

  trace.addPoint(++time, levels, true);
  EXPECT_EQ(2, trace.values().value().size());
  EXPECT_EQ(QVector2D(time, 1), trace.values().value().back());

  trace.addPoint(++time, levels, true);
  EXPECT_EQ(3, trace.values().value().size());
  EXPECT_EQ(QVector2D(time, 1), trace.values().value().back());

  trace.addPoint(++time, levels, true);
  EXPECT_EQ(4, trace.values().value().size());
  EXPECT_EQ(QVector2D(time, 1), trace.values().value().back());

  trace.addPoint(++time, levels, true);
  EXPECT_EQ(5, trace.values().value().size());
  EXPECT_EQ(QVector2D(time, 1), trace.values().value().back());
}

TEST(ScopeTrace, CompressPoints)
{
  // Only storing level changes

  ScopeTrace trace(Qt::red, 1, 1, 0, 10);
  // Add three identical levels
  std::array<int, MAX_DMX_ADDRESS> levels = { 1, 2 };
  float time = 0;
  trace.addPoint(time, levels, false);
  trace.addPoint(++time, levels, false);
  trace.addPoint(++time, levels, false);
  EXPECT_EQ(3, trace.values().value().size());
  EXPECT_EQ(time, trace.values().value().back().x());
  // Should now start compressing points
  trace.addPoint(++time, levels, false);
  EXPECT_EQ(3, trace.values().value().size());
  EXPECT_EQ(time, trace.values().value().back().x());
  // Change level
  levels[0] = 3;
  trace.addPoint(++time, levels, false);
  EXPECT_EQ(4, trace.values().value().size());
  trace.addPoint(++time, levels, false);
  EXPECT_EQ(5, trace.values().value().size());
  EXPECT_EQ(time, trace.values().value().back().x());
  // Should now start compressing points
  trace.addPoint(++time, levels, false);
  EXPECT_EQ(5, trace.values().value().size());
  EXPECT_EQ(time, trace.values().value().back().x());
}

void VerifyRedGreenTrace(ScopeModel& glscope, const char* note)
{
  // There are Two! Traces!
  EXPECT_EQ(2, glscope.rowCount()) << note;

  {
    // Get the Red one
    const ScopeTrace* red_trace = glscope.findTrace(1, 1);
    EXPECT_NE(nullptr, red_trace);
    if (red_trace)
    {
      EXPECT_TRUE(red_trace->isValid()) << note;
      EXPECT_FALSE(red_trace->isSixteenBit()) << note;
      EXPECT_EQ(QColor(Qt::red), red_trace->color()) << note;
      EXPECT_EQ(6, red_trace->values().value().size()) << note;
      // Verify first and last values
      EXPECT_EQ(QVector2D(10, 1), red_trace->values().value().front()) << note;
      EXPECT_EQ(QVector2D(16, 4), red_trace->values().value().back()) << note;
    }
  }

  {
    // Get the Green one
    const ScopeTrace* green_trace = glscope.findTrace(1, 2, 3);
    EXPECT_NE(nullptr, green_trace) << note;
    if (green_trace)
    {
      EXPECT_TRUE(green_trace->isValid()) << note;
      EXPECT_TRUE(green_trace->isSixteenBit()) << note;
      EXPECT_EQ(QColor("green"), green_trace->color()) << note;
      EXPECT_EQ(7, green_trace->values().value().size()) << note;
      // Verify first and last values
      EXPECT_EQ(QVector2D(10, 5), green_trace->values().value().front()) << note;
      EXPECT_EQ(QVector2D(16, 7), green_trace->values().value().back()) << note;
    }
  }
}

TEST(ScopeModel, CSVImportExport)
{
  ScopeModel glscope;

  // Data to load with a duff column
  QByteArray csvData = ("Color,red,brown,green\n"
    "Time (s), U1.1, U1.nope, U1.2/3\n"
    "10, 1, 1, 5\n"
    "11, 2, 1, 6\n"
    "12, 3, 1, 4\n"
    "13, , 1, 7\n"
    "14, 4, 1, 7\n"
    "15, 4, 1, 7\n"
    "16, 4, 1, 7\n");

  {
    QBuffer buf(&csvData);
    buf.open(QIODevice::ReadOnly);

    EXPECT_TRUE(glscope.loadTraces(buf));

    VerifyRedGreenTrace(glscope, "import");
  }

  // Export the data
  {
    QFile csvExport(QStringLiteral("CSVImportExport.csv"));
    csvExport.open(QIODevice::WriteOnly);

    EXPECT_TRUE(glscope.saveTraces(csvExport));
  }

  glscope.removeAllTraces();
  EXPECT_EQ(0, glscope.rowCount()) << "after removeAllTraces";

  {
    // Then re-import and retest
    QFile csvExport(QStringLiteral("CSVImportExport.csv"));
    csvExport.open(QIODevice::ReadOnly);

    EXPECT_TRUE(glscope.loadTraces(csvExport));

    VerifyRedGreenTrace(glscope, "round_trip");
  }
}

TEST(ScopeModel, CaptureConfigImportExport)
{
  const ScopeModel defaultScope;
  ScopeModel scope;

  const QString defaultConfig = scope.captureConfigurationString();
  scope.setCaptureConfiguration(defaultConfig);
  QString currentConfig = scope.captureConfigurationString();
  EXPECT_STREQ(qUtf8Printable(defaultConfig), qUtf8Printable(currentConfig));

  // Write and read trigger setting
  scope.setTriggerType(ScopeModel::Trigger::Above);
  scope.setTriggerLevel(120);
  currentConfig = scope.captureConfigurationString();
  EXPECT_STRNE(qUtf8Printable(defaultConfig), qUtf8Printable(currentConfig));

  scope.setTriggerType(ScopeModel::Trigger::FreeRun);
  scope.setTriggerLevel(0);
  scope.setCaptureConfiguration(currentConfig);

  EXPECT_EQ(ScopeModel::Trigger::Above, scope.triggerType());
  EXPECT_EQ(120, scope.triggerLevel());

  // Write and read run time
  scope.setRunTime(10.0);
  currentConfig = scope.captureConfigurationString();
  EXPECT_STRNE(qUtf8Printable(defaultConfig), qUtf8Printable(currentConfig));

  scope.setRunTime(0);
  scope.setCaptureConfiguration(currentConfig);
  EXPECT_EQ(10.0, scope.runTime());

  // Clear and set trigger again
  scope.setTriggerType(ScopeModel::Trigger::FreeRun);
  scope.setTriggerLevel(0);
  scope.setCaptureConfiguration(currentConfig);

  EXPECT_EQ(ScopeModel::Trigger::Above, scope.triggerType());
  EXPECT_EQ(120, scope.triggerLevel());

}
