#include "qspinbox_resizetocontent.h"
#include <QStyleOptionSpinBox>

QSpinBox_ResizeToContent::QSpinBox_ResizeToContent(QWidget *parent) : QSpinBox(parent) {
    connect(this, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]() { setMinimumSize(sizeHint()); });
}

QSize QSpinBox_ResizeToContent::sizeHint() const {
    QStyleOptionSpinBox option;
    option.initFrom(this);
    auto strWidth = fontMetrics().horizontalAdvance(text());
    auto strPadding = (fontMetrics().horizontalAdvance(QChar(32)) * 2) * 2; // Two spaces either side
    auto upBtnWidth = style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, this).width();
    auto downBtnWidth = style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxDown, this).width();

    auto hint = QSpinBox::sizeHint();
    hint.setWidth(strWidth
                  + strPadding
                  + upBtnWidth
                  + downBtnWidth);
    return hint;
}
