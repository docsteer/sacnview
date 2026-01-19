#include "qspinbox_resizetocontent.h"
#include <QStyleOptionSpinBox>

#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
# define horizontalAdvance fontMetrics().width
#else
# define horizontalAdvance fontMetrics().horizontalAdvance
#endif

QSpinBox_ResizeToContent::QSpinBox_ResizeToContent(QWidget * parent)
    : QSpinBox(parent)
{
    connect<void (QSpinBox::*)(int)>(this, &QSpinBox::valueChanged, this, [=]() { setMinimumSize(minimumSizeHint()); });
}

QSize QSpinBox_ResizeToContent::minimumSizeHint() const
{
    QStyleOptionSpinBox option;
    option.initFrom(this);
    auto strWidth = horizontalAdvance(text());
    auto strPadding = (horizontalAdvance(QChar(32)) * 2) * 2; // Two spaces either side
    auto upBtnWidth = style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, this).width();
    auto downBtnWidth = style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxDown, this).width();

    auto hint = QSpinBox::minimumSizeHint();
    hint.setWidth(strWidth + strPadding + upBtnWidth + downBtnWidth);
    return hint;
}
