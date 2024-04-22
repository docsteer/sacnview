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

#include "sacn/fpscounter.h"

// Custom wrapper to allow external ticking
class T_FpsCounter : public FpsCounter
{
public:
  T_FpsCounter() : FpsCounter() {}
  void timerEvent() { FpsCounter::timerEvent(nullptr); }
};

TEST(FpsCounter, Empty)
{
  T_FpsCounter fps;
  EXPECT_EQ(0.0f, fps.FPS());
  EXPECT_FALSE(fps.isNewFPS());
  EXPECT_EQ(0u, fps.GetHistogram().size());

  // Refresh, should still be empty
  fps.timerEvent();

  EXPECT_FALSE(fps.isNewFPS());
  EXPECT_EQ(0.0f, fps.FPS());
  EXPECT_EQ(0u, fps.GetHistogram().size());
}

TEST(FpsCounter, SingleFps)
{
  T_FpsCounter fps;

  tock timestamp;
  constexpr FpsCounter::HistogramBucket ms10 = std::chrono::milliseconds(10);
  constexpr FpsCounter::HistogramBucket ms20 = std::chrono::milliseconds(20);

  // Add one tick
  fps.newFrame(timestamp);
  fps.timerEvent();
  // Should still be empty
  EXPECT_FALSE(fps.isNewFPS());
  EXPECT_EQ(0.0f, fps.FPS());
  EXPECT_EQ(0u, fps.GetHistogram().size());

  // Add a second tick after 20ms
  timestamp.Set(timestamp.Get() + ms20);
  fps.newFrame(timestamp);
  fps.timerEvent();

  // Should now have 50 FPS
  EXPECT_TRUE(fps.isNewFPS());
  EXPECT_FLOAT_EQ(50.0f, fps.FPS());
  // And one interval value
  EXPECT_EQ(1u, fps.GetHistogram().size());

  // Add a second tick after 20ms
  timestamp.Set(timestamp.Get() + ms20);
  fps.newFrame(timestamp);
  fps.timerEvent();
  // Still 50 FPS
  EXPECT_FALSE(fps.isNewFPS());
  EXPECT_FLOAT_EQ(50.0f, fps.FPS());
  EXPECT_EQ(1u, fps.GetHistogram().size());

  // And a third tick after 10ms
  timestamp.Set(timestamp.Get() + ms10);
  fps.newFrame(timestamp);
  fps.timerEvent();
  // Not 50 FPS
  EXPECT_TRUE(fps.isNewFPS());
  EXPECT_LT(50.0f, fps.FPS());
  EXPECT_FALSE(fps.isNewFPS());

  {
    // Have seen values, 10 and 20
    const auto hist = fps.GetHistogram();
    EXPECT_EQ(2u, hist.size());
    auto it = hist.begin();
    // 10ms once
    EXPECT_EQ(ms10, (*it).first);
    EXPECT_EQ(1u, (*it).second);
    // 20ms twice
    ++it;
    if (it != hist.end())
    {
      EXPECT_EQ(ms20, (*it).first);
      EXPECT_EQ(2u, (*it).second);
    }
  }

  // Clear the histogram and confirm empty
  fps.ClearHistogram();
  EXPECT_EQ(0u, fps.GetHistogram().size());
  fps.timerEvent();
  EXPECT_EQ(0u, fps.GetHistogram().size());
}

TEST(FpsCounter, ExpectedPacketRefresh)
{
  T_FpsCounter fps;

  tock timestamp;
  constexpr FpsCounter::HistogramBucket ms10 = std::chrono::milliseconds(10);
  constexpr FpsCounter::HistogramBucket ms20 = std::chrono::milliseconds(20);

  // Add 1 second worth of 20ms intervals
  while (timestamp < std::chrono::seconds(1))
  {
    timestamp.Set(timestamp.Get() + ms20);
    fps.newFrame(timestamp);
  }

  // Haven't updated yet so should appear empty
  EXPECT_FALSE(fps.isNewFPS());
  EXPECT_EQ(0.0f, fps.FPS());
  EXPECT_EQ(0u, fps.GetHistogram().size());

  // Now tick
  fps.timerEvent();

  // Should now have 50 FPS
  EXPECT_TRUE(fps.isNewFPS());
  EXPECT_FLOAT_EQ(50.0f, fps.FPS());
  // And one interval value
  EXPECT_EQ(1u, fps.GetHistogram().size());

  // And do it again, but faster
  // Add 1 more second worth of 10ms intervals
  while (timestamp < std::chrono::seconds(2))
  {
    timestamp.Set(timestamp.Get() + ms10);
    fps.newFrame(timestamp);
  }

  // Haven't updated yet so should appear unchanged
  EXPECT_FALSE(fps.isNewFPS());
  EXPECT_FLOAT_EQ(50.0f, fps.FPS());
  EXPECT_EQ(1u, fps.GetHistogram().size());

  // Now tick
  fps.timerEvent();

  // Should now have 100 FPS as the entire sample period was 10ms intervals
  EXPECT_TRUE(fps.isNewFPS());
  EXPECT_FLOAT_EQ(100.0f, fps.FPS());
  // And two interval values
  EXPECT_EQ(2u, fps.GetHistogram().size());
}

#if 0
// Temporary test used to verify sequence error behavior
// Only valid if gcc/clang -fwrapv or equivalent is set
TEST(SequenceNumber, Jumps)
{
  for (size_t seq = 0; seq < 256; ++seq)
  {
    for (size_t prev = 0; prev < 256; ++prev)
    {
      const uint8_t sequence = static_cast<uint8_t>(seq);
      const uint8_t lastseq = static_cast<uint8_t>(prev);
      bool jump_signed = false;
      bool seqerr_signed = false;
      // gcc/clang -fwrapv implementation
      // Invokes undefined behavior on other toolchains
      const qint8 wrapresult = reinterpret_cast<const qint8&>(sequence) - reinterpret_cast<const qint8&>(lastseq);
      if (wrapresult != 1)
      {
        jump_signed = true;
        if ((wrapresult <= 0) && (wrapresult > -20))
        {
          seqerr_signed = true;
        }
      }

      bool jump_unsigned = false;
      bool seqerr_unsigned = false;

      // MSVC 2015 onwards cannot rely on signed overflow wrapping
      // https://devblogs.microsoft.com/cppblog/new-code-optimizer/
      // (The switches mentioned in the blog post were temporary and no longer available)
      const uint8_t result = sequence - lastseq;
      if (result != 1)
      {
        jump_unsigned = true;
        if (result == 0 || result > 236) // Unsigned representation of two's complement "-20"
        {
          seqerr_unsigned = true;
        }
      }

      EXPECT_EQ(jump_signed, jump_unsigned) << prev << '>' << seq;
      EXPECT_EQ(seqerr_signed, seqerr_unsigned) << prev << '>' << seq;
    }
  }
}
#endif
