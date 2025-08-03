#ifndef BIGDISPLAY_H
#define BIGDISPLAY_H

#include <QWidget>
#include "consts.h"
#include "streamingacn.h"

namespace Ui {
  class BigDisplay;
}

class BigDisplay : public QWidget
{
  Q_OBJECT

public:
  explicit BigDisplay(int universe, quint16 slot_index, QWidget* parent = 0);
  ~BigDisplay();

private:
  Ui::BigDisplay* ui;

  enum tabModes
  {
    tabModes_bit8,
    tabModes_bit16,
    tabModes_rgb
  };

protected:
  void timerEvent(QTimerEvent* ev) override;

private slots:
  void on_tabWidget_currentChanged(int index);

private:
  void updateLevels();
  void displayLevel();
  sACNManager::tListener m_listener;
  int m_level = 0;
  bool m_active = false;
};

#endif // BIGDISPLAY_H
