#include "qpushbutton_rightclick.h"

QPushButton_RightClick::QPushButton_RightClick(QWidget * parent)
    : QPushButton(parent)
{}

void QPushButton_RightClick::mousePressEvent(QMouseEvent * e)
{
    if (e->button() == Qt::RightButton) emit rightClicked();

    QPushButton::mousePressEvent(e);
}
