#ifndef QPUSHBUTTON_RIGHTCLICK_H
#define QPUSHBUTTON_RIGHTCLICK_H

#include <QMouseEvent>
#include <QPushButton>

class QPushButton_RightClick : public QPushButton
{
    Q_OBJECT
public:

    explicit QPushButton_RightClick(QWidget * parent = 0);

private slots:
    void mousePressEvent(QMouseEvent * e);

signals:
    void rightClicked();
};

#endif // QPUSHBUTTON_RIGHTCLICK_H
