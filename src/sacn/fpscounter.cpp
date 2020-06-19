#include "fpscounter.h"

#define updateInterval 1000

FpsCounter::FpsCounter(QObject *parent) : QObject(parent),
    currentFps(0),
    previousFps(0),
    lastTime(0)
{
    // Maximum fps permitted by standard is 44, so this should never reallocate
    frameTimes.reserve(50);
    elapsedTimer.start();
    timerId = startTimer(updateInterval);
}

FpsCounter::~FpsCounter()
{
    if (timerId != 0)
        killTimer(timerId);
}

void FpsCounter::timerEvent(QTimerEvent * /*e*/)
{
    QMutexLocker queueLocker(&queueMutex);

    // We need at least two frames to calculate interval
    if (Q_UNLIKELY(frameTimes.count() < 2))
    {
        // No frames, or very old single frame
        if (
            (frameTimes.count() == 0) ||
            ((frameTimes.count() == 1) && (elapsedTimer.elapsed() > (frameTimes.back() + (updateInterval * 2))))
            )
        {
            previousFps = currentFps;
            currentFps = 0;
        }
    } else {
        if (lastTime == 0)
            lastTime = frameTimes.takeFirst();

        // Create list of all intervals
        QList<qint64> intervals;
        while (frameTimes.count())
        {
            auto time = frameTimes.takeFirst();

            if (time > lastTime)
            {
                intervals << time - lastTime;
                lastTime = time;
            }
        }

        if (intervals.isEmpty()) return;

        // Calculate average of the intervals
        qint64 intervalTotal = 0;
        for (auto interval: intervals)
        {
            intervalTotal += interval;
        }
        auto intervalAvg = intervalTotal / intervals.count();

        // Calculate the current FPS
        previousFps = currentFps;
        currentFps = 1000 / static_cast<float>(intervalAvg);
    }

    // Flag if the FPS has changed
    newFps = ((previousFps < currentFps) | (previousFps > currentFps));
}

void FpsCounter::newFrame()
{
    QMutexLocker queueLocker(&queueMutex);
    frameTimes.append(elapsedTimer.elapsed());
}
