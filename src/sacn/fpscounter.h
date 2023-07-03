#ifndef FPSCounter_H
#define FPSCounter_H

#include <QObject>
#include <QMutexLocker>

#include "tock.h"

#include <vector>

class FpsCounter : public QObject
{
  Q_OBJECT
public:
  explicit FpsCounter(QObject* parent = nullptr);
  ~FpsCounter();

  // Return current FPS
  float FPS() const { m_newFps = false; return m_currentFps; }

  // Returns true if FPS has changed since last checked
  bool isNewFPS() const { return m_newFps; }

  // Log new frame
  void newFrame(tock timePoint);

signals:
  void updatedFPS();

protected:
  void timerEvent(QTimerEvent* ev) final;

private:
  int m_timerId = 0;

  float m_currentFps = 0.0f;
  float m_previousFps = 0.0f;
  mutable bool m_newFps = false;

  QMutex m_queueMutex;
  std::vector<tock::resolution_t> m_frameTimes;
  tock::resolution_t m_lastFrameTime{0};
};

#endif // FPSCounter_H
