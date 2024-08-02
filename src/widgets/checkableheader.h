#pragma once

#include <QHeaderView>

class CheckableHeader : public QHeaderView
{
  Q_OBJECT

public:
  CheckableHeader(QWidget* parent = nullptr);

protected:
  QSize sectionSizeFromContents(int logicalIndex) const override;
  void	paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;

  void	mousePressEvent(QMouseEvent* e) override;
  void	mouseReleaseEvent(QMouseEvent* e) override;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  Q_SLOT void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) override;
#else
  Q_SLOT void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles = QList<int>()) override;
#endif

  // Cache of the section rects for mouse actions
  mutable QMap<int, QRect> m_sectionRects;
  int m_checkMouseDownIndex = -1;

private:
  bool sectionIsCheckable(int logicalIndex) const;
  QPixmap drawCheckbox(Qt::CheckState checkState) const;
};
