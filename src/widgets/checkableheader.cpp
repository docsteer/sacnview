#include "checkableheader.h"

#include <QMouseEvent>
#include <QPainter>

CheckableHeader::CheckableHeader(QWidget* parent)
  : QHeaderView(Qt::Horizontal, parent)
{
  setSectionsClickable(true);
}

QSize CheckableHeader::sectionSizeFromContents(int logicalIndex) const
{
  // If this section is checkable, set the icon first
  // Immediately update the pixmap for this item
  const QVariant checkVariant = model()->headerData(logicalIndex, orientation(), Qt::CheckStateRole);
  if (checkVariant.canConvert<Qt::CheckState>())
  {
    const Qt::CheckState checkState = checkVariant.value<Qt::CheckState>();
    model()->setHeaderData(logicalIndex, orientation(), drawCheckbox(checkState), Qt::DecorationRole);
  }

  return QHeaderView::sectionSizeFromContents(logicalIndex);
}

void CheckableHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
  // Update section rect cache for mouse actions
  m_sectionRects[logicalIndex] = rect;
  QHeaderView::paintSection(painter, rect, logicalIndex);
}

void CheckableHeader::mousePressEvent(QMouseEvent* e)
{
  // if checkable, check/uncheck on left instead of normal functions
  if (e->button() == Qt::LeftButton)
  {
    const int logicalIndex = logicalIndexAt(e->pos());
    if (sectionIsCheckable(logicalIndex))
    {
      QRect sectionRect = m_sectionRects.value(logicalIndex);
      // Left-hand third?
      const int xpos = e->pos().x();
      if (xpos > sectionRect.left() && xpos < (sectionRect.left() + (sectionRect.width() / 3)))
      {
        m_checkMouseDownIndex = logicalIndex;
        return;
      }
    }
  }

  QHeaderView::mousePressEvent(e);
}

void CheckableHeader::mouseReleaseEvent(QMouseEvent* e)
{
  // check/uncheck on left instead if checkable
  if (e->button() == Qt::LeftButton && m_checkMouseDownIndex != -1)
  {
    const QVariant checkVariant = model()->headerData(m_checkMouseDownIndex, orientation(), Qt::CheckStateRole);
    if (checkVariant.canConvert<Qt::CheckState>())
    {
      const Qt::CheckState checkState = checkVariant.value<Qt::CheckState>();
      model()->setHeaderData(m_checkMouseDownIndex, orientation(), checkState == Qt::Unchecked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
    }
    m_checkMouseDownIndex = -1;
    return;
  }

  QHeaderView::mouseReleaseEvent(e);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void CheckableHeader::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
#else
void CheckableHeader::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles)
#endif
{
  if (roles.empty() || roles.contains(Qt::CheckStateRole))
  {
    // Immediately update the pixmap for this item
    const int logicalIndex = topLeft.column();
    const QVariant checkVariant = model()->headerData(logicalIndex, orientation(), Qt::CheckStateRole);
    if (checkVariant.canConvert<Qt::CheckState>())
    {
      const Qt::CheckState checkState = checkVariant.value<Qt::CheckState>();
      model()->setHeaderData(logicalIndex, orientation(), drawCheckbox(checkState), Qt::DecorationRole);
    }
    else
    {
      model()->setHeaderData(logicalIndex, orientation(), QVariant(), Qt::DecorationRole);
    }
  }

  QHeaderView::dataChanged(topLeft, bottomRight, roles);
}

bool CheckableHeader::sectionIsCheckable(int logicalIndex) const
{
  const QVariant checkVariant = model()->headerData(logicalIndex, orientation(), Qt::CheckStateRole);
  return (!checkVariant.isNull());
}

QPixmap CheckableHeader::drawCheckbox(Qt::CheckState checkState) const
{
  QStyleOptionButton option;

  if (isEnabled() && sectionsClickable())
    option.state |= QStyle::State_Enabled;

  QStyleOptionButton checkBoxStyleOption;
  option.rect = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &checkBoxStyleOption);

  switch (checkState)
  {
  case Qt::Unchecked:option.state |= QStyle::State_Off; break;
  case Qt::PartiallyChecked:option.state |= QStyle::State_NoChange; break;
  case Qt::Checked:    option.state |= QStyle::State_On; break;
  }

  QPixmap pixmap(option.rect.size());
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.translate(-option.rect.topLeft()); // We only want to draw the active part
  style()->drawControl(QStyle::CE_CheckBox, &option, &painter);
  return pixmap;
}
