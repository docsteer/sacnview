// Copyright 2025 Tom Steer
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#include "resettablecounterdelegate.h"

void ResettableCounterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const auto widget = opt.widget;
    const auto style = widget ? widget->style() : QApplication::style();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
}

bool ResettableCounterDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
    const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        const auto mouseEvent = static_cast<QMouseEvent*>(event);
        auto opt = option;
        initStyleOption(&opt, index);

        const auto widget = opt.widget;
        const auto iconRect = widget->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, widget);

        if (iconRect.isValid() && iconRect.contains(mouseEvent->pos()))
        {
            model->setData(index, QVariant(0));
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void ResettableCounterDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->icon = QIcon(":/icons/clear.png");
    option->decorationSize = QSize(16, 16);
    option->showDecorationSelected = true;
    option->displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    option->decorationPosition = QStyleOptionViewItem::Right;
    option->features = QStyleOptionViewItem::HasDecoration;
}