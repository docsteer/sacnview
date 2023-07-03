#include "multiview.h"

#include "ui_multiview.h"

#include "consts.h"
#include "models/sacnsourcetablemodel.h"
#include "models/csvmodelexport.h"

#include <QFileDialog>
#include <QStandardPaths>

MultiView::MultiView(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::MultiView)
  , m_sourceTableModel(new SACNSourceTableModel(this))
{
  ui->setupUi(this);
  ui->sourceTableView->setModel(m_sourceTableModel);
  ui->sourceTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  ui->spinUniverseMin->setMinimum(MIN_SACN_UNIVERSE);
  ui->spinUniverseMin->setMaximum(MAX_SACN_UNIVERSE);

  ui->spinUniverseMax->setMinimum(MIN_SACN_UNIVERSE);
  ui->spinUniverseMax->setMaximum(MAX_SACN_UNIVERSE);
}

MultiView::~MultiView()
{
  delete ui;
}

void MultiView::on_btnStartStop_clicked(bool checked)
{
  if (checked)
  {
    ui->spinUniverseMin->setEnabled(false);
    ui->spinUniverseMax->setEnabled(false);

    // Clear and restart listening for the large number of universes
    m_sourceTableModel->resetCounters();
    m_sourceTableModel->clear();
    
    // Hold onto the old listeners so any overlaps will not be destructed
    std::map<uint16_t, sACNManager::tListener> old_listeners;
    old_listeners.swap(m_listeners);

    const uint16_t minUniverse = static_cast<uint16_t>(ui->spinUniverseMin->value());
    const uint16_t maxUniverse = static_cast<uint16_t>(ui->spinUniverseMax->value());
    for (uint16_t universe = minUniverse; universe < maxUniverse; ++universe)
    {
      auto listener = sACNManager::Instance().getListener(universe);
      m_sourceTableModel->addListener(listener);
      m_listeners.emplace(universe, listener);
    }

    // Unused listeners will now go out of scope and be destroyed "later"
  }
  else
  {
    ui->spinUniverseMin->setEnabled(true);
    ui->spinUniverseMax->setEnabled(true);
    m_sourceTableModel->pause();
  }
}

void MultiView::on_btnResetCounters_clicked()
{
  m_sourceTableModel->resetCounters();
}

void MultiView::on_btnExport_clicked()
{
  const QString filename = QFileDialog::getSaveFileName(this, tr("Export Sources Table"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), QStringLiteral("*.csv"));

  if (filename.isEmpty())
    return;

  // Export as CSV
  CsvModelExporter csv_export(m_sourceTableModel);
  csv_export.saveAs(filename);
}
