#ifndef BIGDISPLAY_H
#define BIGDISPLAY_H

#include <QWidget>
#include "consts.h"

namespace Ui {
class BigDisplay;
}

class BigDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit BigDisplay(int universe, quint16 address, QWidget *parent = 0);
    ~BigDisplay();

private:
    Ui::BigDisplay *ui;

    enum tabModes
    {
        tabModes_bit8,
        tabModes_bit16,
        tabModes_rgb
    };

private slots:
    void dataReady(int universe, quint16 address, QPointF data);

    void on_tabWidget_currentChanged(int index);

private:
    void displayLevel();

    quint32 m_level;
};

#endif // BIGDISPLAY_H
