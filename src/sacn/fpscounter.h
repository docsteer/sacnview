#ifndef FPSCounter_H
#define FPSCounter_H

#include <QObject>
#include <QList>
#include <QElapsedTimer>
#include <QMutexLocker>

class FpsCounter : public QObject
{
    Q_OBJECT
public:
    explicit FpsCounter(QObject *parent = nullptr);
    ~FpsCounter();

    // Return current FPS
    float FPS() const { newFps = false; return currentFps;}

    // Returns true if FPS has changed since last checked
    bool isNewFPS() const { return newFps; }

    // Log new frame
    void newFrame();

protected:
    void timerEvent(QTimerEvent *e) final;

private:
    QElapsedTimer elapsedTimer;
    int timerId = 0;

    float currentFps = 0.0f;
    float previousFps = 0.0f;
    mutable bool newFps = false;

    QMutex queueMutex;
    QVector<qint64> frameTimes;
    qint64 lastTime = 0;
};

#endif // FPSCounter_H
