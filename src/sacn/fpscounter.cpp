#include "fpscounter.h"
#include <QDateTime>

fpsCounter::fpsCounter(QObject *parent) : QObject(parent),
    updateTimer(new QTimer(this)),
    currentFps(0),
    previousFps(0),
    lastTime(0)
{
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateFPS()));
    updateTimer->start(1000);
}

fpsCounter::~fpsCounter()
{
    updateTimer->deleteLater();
}

void fpsCounter::updateFPS()
{
    QMutexLocker queueLocker(&queueMutex);

    // We need at least two frame to calculate interval
    if (frameTimes.count() < 2)
    {
        // No frames, or very old single frame
        if (
                (frameTimes.count() == 0) ||
                ((frameTimes.count() == 1) && (QDateTime::currentMSecsSinceEpoch() > (frameTimes.back() + (updateTimer->interval() * 2))))
            )
        {
            previousFps = currentFps;
            currentFps = 0;
        }
    } else {
        if (lastTime == 0)
            lastTime = frameTimes.takeFirst();

        // Create list of all intervals
        QList<time_t> intervals;
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
        time_t intervalTotal = 0;
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

void fpsCounter::newFrame()
{
    QMutexLocker queueLocker(&queueMutex);
    frameTimes.append(QDateTime::currentMSecsSinceEpoch());
}
