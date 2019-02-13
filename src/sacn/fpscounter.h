#ifndef FPSCounter_H
#define FPSCounter_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QMutexLocker>

class fpsCounter : public QObject
{
    Q_OBJECT
public:
    explicit fpsCounter(QObject *parent = nullptr);

    // Return current FPS
    float FPS() { newFps = false; return currentFps;}

    // Returns true if FPS has changed since last checked
    bool isNewFPS() { return newFps; }

    // Log new frame
    void newFrame();

private slots:
    void updateFPS();

private:
    typedef time_t quint64;

    float currentFps;
    float previousFps;
    bool newFps;

    QMutex queueMutex;
    QList<time_t> frameTimes;
    time_t lastTime;
};

#endif // FPSCounter_H
