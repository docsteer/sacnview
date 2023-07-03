#include "fpscounter.h"

#include "streamingacn.h"

constexpr int updateInterval = 1000;

FpsCounter::FpsCounter(QObject* parent) : QObject(parent)
{
  // Maximum fps permitted by standard is 44, so this should never reallocate
  m_frameTimes.reserve(50);
  m_timerId = startTimer(updateInterval);
}

FpsCounter::~FpsCounter()
{
  if (m_timerId != 0)
    killTimer(m_timerId);
}

void FpsCounter::timerEvent(QTimerEvent* /*ev*/)
{
  QMutexLocker queueLocker(&m_queueMutex);

  // We need at least two frames to calculate interval
  if (Q_UNLIKELY(m_frameTimes.size() < 2))
  {
    // No frames, or very old single frame
    if (
      m_frameTimes.empty() ||
      ((m_frameTimes.size() == 1) && (sACNManager::GetTock().Get() > (m_frameTimes.back() + std::chrono::milliseconds(updateInterval * 2))))
      )
    {
      m_previousFps = m_currentFps;
      m_currentFps = 0;
    }
  }
  else
  {
    // Calculate average of the intervals
    tock::resolution_t intervalTotal{0};
    int64_t intervalCount = 0;

    size_t frameIndex = 0;
    if (m_lastFrameTime == tock::resolution_t::zero())
    {
      m_lastFrameTime = m_frameTimes[0];
      frameIndex = 1;
    }

    for (/* init above */; frameIndex < m_frameTimes.size(); ++frameIndex)
    {
      const auto& time = m_frameTimes[frameIndex];

      if (time > m_lastFrameTime)
      {
        const auto interval = time - m_lastFrameTime;
        m_lastFrameTime = time;
        intervalTotal += interval;
        ++intervalCount;
      }
    }

    m_frameTimes.clear();

    if (intervalCount == 0)
      return;

    const auto intervalAvg = intervalTotal / intervalCount;

    // Calculate the current FPS
    m_previousFps = m_currentFps;
    m_currentFps = 1.0f / std::chrono::duration<float>(intervalAvg).count();
  }

  queueLocker.unlock();

  // Flag if the FPS has changed
  m_newFps = (m_previousFps < m_currentFps) || (m_previousFps > m_currentFps);

  if (m_newFps)
  {
    emit updatedFPS();
  }
}

void FpsCounter::newFrame(tock timePoint)
{
  QMutexLocker queueLocker(&m_queueMutex);
  m_frameTimes.push_back(timePoint.Get());
}
