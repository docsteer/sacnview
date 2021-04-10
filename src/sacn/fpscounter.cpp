#include "fpscounter.h"

#define updateInterval 1000

FpsCounter::FpsCounter(QObject *parent) : QObject(parent)
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
        // Calculate average of the intervals
        qint64 intervalTotal = 0;
        qint64 intervalCount = 0;

        int frameIndex = 0;
        if (lastFrameTime == 0)
        {
            lastFrameTime = frameTimes[0];
            frameIndex = 1;
        }

        for (/* init above */ ; frameIndex < frameTimes.count() ; ++frameIndex)
        {
            auto time = frameTimes[frameIndex];

            if (time > lastFrameTime)
            {
                auto interval = time - lastFrameTime;
                lastFrameTime = time;
                intervalTotal += interval;
                ++intervalCount;
            }
        }

        frameTimes.clear();

        if (intervalCount == 0)
            return;

        auto intervalAvg = intervalTotal / intervalCount;

        // Calculate the current FPS
        previousFps = currentFps;
        currentFps = 1000 / static_cast<float>(intervalAvg);
    }

    // Flag if the FPS has changed
    newFps = ((previousFps < currentFps) | (previousFps > currentFps));

    if (newFps)
        emit updatedFPS();
}

void FpsCounter::newFrame()
{
    qint64 elapsedTime = elapsedTimer.elapsed();
    QMutexLocker queueLocker(&queueMutex);
    frameTimes.append(elapsedTime);
    // TODO: Consider using an IIR filter to produce a rolling average
}
