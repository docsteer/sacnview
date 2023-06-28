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
  ScopeTrace trace(Qt::red, 1, 1, 0, 10);
  EXPECT_TRUE(trace.isValid());
  EXPECT_FALSE(trace.isSixteenBit());
  EXPECT_TRUE(trace.values().empty());
  const std::array<int, MAX_DMX_ADDRESS> levels = { 1, 2 };
  trace.addPoint(0, levels);
  ASSERT_EQ(1, trace.values().size());
  EXPECT_EQ(QVector2D(0, 1), trace.values()[0]);
}

TEST(ScopeTrace, CompressPoints)
{
  ScopeTrace trace(Qt::red, 1, 1, 0, 10);
  
  // Add three identical levels
  std::array<int, MAX_DMX_ADDRESS> levels = { 1, 2 };
  float time = 0;
  trace.addPoint(time, levels);
  trace.addPoint(++time, levels);
  trace.addPoint(++time, levels);
  EXPECT_EQ(3, trace.values().size());
  EXPECT_EQ(time, trace.values().back().x());
  // Should now start compressing points
  trace.addPoint(++time, levels);
  EXPECT_EQ(3, trace.values().size());
  EXPECT_EQ(time, trace.values().back().x());
  // Change level
  levels[0] = 3;
  trace.addPoint(++time, levels);
  EXPECT_EQ(4, trace.values().size());
  trace.addPoint(++time, levels);
  EXPECT_EQ(5, trace.values().size());
  EXPECT_EQ(time, trace.values().back().x());
  // Should now start compressing points
  trace.addPoint(++time, levels);
  EXPECT_EQ(5, trace.values().size());
  EXPECT_EQ(time, trace.values().back().x());
}

void VerifyRedGreenTrace(const ScopeModel& glscope, const char* note)
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
      EXPECT_EQ(6, red_trace->values().size()) << note;
      // Verify first and last values
      EXPECT_EQ(QVector2D(10, 1), red_trace->values().front()) << note;
      EXPECT_EQ(QVector2D(16, 4), red_trace->values().back()) << note;
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
      EXPECT_EQ(7, green_trace->values().size()) << note;
      // Verify first and last values
      EXPECT_EQ(QVector2D(10, 5), green_trace->values().front()) << note;
      EXPECT_EQ(QVector2D(16, 7), green_trace->values().back()) << note;
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
