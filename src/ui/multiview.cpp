#include "multiview.h"

#include "ui_multiview.h"

MultiView::MultiView(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::MultiView)
{
  // Go crazy and listen to like a hundred universes for giggles
  for (uint16_t universe = 1; universe < 100; ++universe)
    m_listeners.emplace(universe, sACNManager::Instance().getListener(universe));
}

MultiView::~MultiView()
{
  delete ui;
}
