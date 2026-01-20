#include "fpscounter.h"

#include "streamingacn.h"

constexpr std::chrono::milliseconds updateInterval(1000);

FpsCounter::FpsCounter(QObject * parent)
    : QObject(parent)
{
    // Maximum fps permitted by standard is 44, so this should never reallocate
    m_frameTimes.reserve(127);
    m_timerId = startTimer(updateInterval.count());
}

FpsCounter::~FpsCounter()
{
    if (m_timerId != 0) killTimer(m_timerId);
}

void FpsCounter::timerEvent(QTimerEvent * /*ev*/)
{
    QMutexLocker queueLocker(&m_queueMutex);

    // We need at least two frames to calculate interval
    if (Q_UNLIKELY(m_frameTimes.size() < 2))
    {
        // No frames, or very old single frame
        if (m_frameTimes.empty()
            || ((m_frameTimes.size() == 1)
                && (sACNManager::GetTock().Get() > (m_frameTimes.back() + (updateInterval * 2)))))
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

        tock::resolution_t lastFrameTime = m_frameTimes.front();

        for (size_t frameIndex = 1; frameIndex < m_frameTimes.size(); ++frameIndex)
        {
            const auto & time = m_frameTimes[frameIndex];

            if (time > lastFrameTime)
            {
                const auto interval = time - lastFrameTime;
                lastFrameTime = time;
                // Ignore very long intervals
                if (interval < (updateInterval * 2))
                {
                    // Add to histogram
                    ++m_frameDeltaHistogram[std::chrono::ceil<HistogramBucket>(interval)];
                    // Add to total
                    intervalTotal += interval;
                    ++intervalCount;
                }
            }
        }

        if (intervalCount == 0) return;

        m_frameTimes.clear();
        m_frameTimes.push_back(lastFrameTime);

        const auto intervalAvg = intervalTotal / intervalCount;

        // Calculate the current FPS
        m_previousFps = m_currentFps;
        m_currentFps = 1.0f / std::chrono::duration<float>(intervalAvg).count();
    }

    // Flag if the FPS has changed
    m_newFps = (m_previousFps < m_currentFps) || (m_previousFps > m_currentFps);

    queueLocker.unlock();

    if (m_newFps)
    {
        emit updatedFPS();
    }
}

FpsCounter::Histogram FpsCounter::GetHistogram() const
{
    QMutexLocker queueLocker(&m_queueMutex);
    return m_frameDeltaHistogram;
}

void FpsCounter::ClearHistogram()
{
    QMutexLocker queueLocker(&m_queueMutex);
    m_frameDeltaHistogram.clear();
}

void FpsCounter::newFrame(tock timePoint)
{
    QMutexLocker queueLocker(&m_queueMutex);
    m_frameTimes.push_back(timePoint.Get());
}
