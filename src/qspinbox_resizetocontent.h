#ifndef QSPINBOX_RESIZETOCONTENT_H
#define QSPINBOX_RESIZETOCONTENT_H
#include <QSpinBox>

class QSpinBox_ResizeToContent : public QSpinBox
{
public:
    QSpinBox_ResizeToContent(QWidget *parent = nullptr);
    QSize sizeHint() const;
};

#endif // QSPINBOX_RESIZETOCONTENT_H
