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
  // Histogram bucket size
  using HistogramBucket = std::chrono::milliseconds;
  using Histogram = std::map<HistogramBucket, size_t>;

public:
  explicit FpsCounter(QObject* parent = nullptr);
  ~FpsCounter();

  // Return current FPS
  float FPS() const { m_newFps = false; return m_currentFps; }

  // Returns true if FPS has changed since last checked
  bool isNewFPS() const { return m_newFps; }

  // Get a copy of the frame time histogram
  Histogram GetHistogram() const;
  
  // Clear the histogram
  void ClearHistogram();

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

  mutable QMutex m_queueMutex;
  std::vector<tock::resolution_t> m_frameTimes;
  Histogram m_frameDeltaHistogram;
};

#endif // FPSCounter_H
