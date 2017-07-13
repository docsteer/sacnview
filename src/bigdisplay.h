#ifndef BIGDISPLAY_H
#define BIGDISPLAY_H

#include <QWidget>
#include "consts.h"
#include "sacnlistener.h"

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
    int m_universe;
    QSharedPointer<sACNListener> m_listener;

    enum tabModes
    {
        tabModes_bit8,
        tabModes_bit16,
        tabModes_rgb
    };

    void setupAddressMonitors();

private slots:
    void dataReady(int address, QPointF data);

    void on_spinBox_8_editingFinished();
    void on_spinBox_16_Coarse_editingFinished();
    void on_spinBox_16_Fine_editingFinished();
    void on_spinBox_Colour_1_editingFinished();
    void on_spinBox_Colour_2_editingFinished();
    void on_spinBox_Colour_3_editingFinished();
};

#endif // BIGDISPLAY_H
