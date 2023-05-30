#include "multiview.h"

#include "ui_multiview.h"

#include "models/sacnsourcetablemodel.h"

MultiView::MultiView(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::MultiView)
  , m_sourceTableModel(new SACNSourceTableModel(this))
{
  ui->setupUi(this);
  ui->sourceTableView->setModel(m_sourceTableModel);
  ui->sourceTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  // Go crazy and listen to like a hundred universes for giggles
  for (uint16_t universe = 1; universe < 100; ++universe)
  {
    auto listener = sACNManager::Instance().getListener(universe);
    m_sourceTableModel->addListener(listener);
    m_listeners.emplace(universe, listener);
  }
}

MultiView::~MultiView()
{
  delete ui;
}
