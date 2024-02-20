#include "multiview.h"

#include "ui_multiview.h"

#include "consts.h"
#include "preferences.h"
#include "models/sacnsourcetablemodel.h"
#include "models/csvmodelexport.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QSortFilterProxyModel>

MultiView::MultiView(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::MultiView)
  , m_sourceTableModel(new SACNSourceTableModel(this))
{
  ui->setupUi(this);
  QSortFilterProxyModel* sortProxy = new QSortFilterProxyModel(this);
  sortProxy->setSourceModel(m_sourceTableModel);
  ui->sourceTableView->setModel(sortProxy);
  ui->sourceTableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

  ui->spinUniverseMin->setMinimum(MIN_SACN_UNIVERSE);
  ui->spinUniverseMin->setMaximum(MAX_SACN_UNIVERSE);

  ui->spinUniverseMax->setMinimum(MIN_SACN_UNIVERSE);
  ui->spinUniverseMax->setMaximum(MAX_SACN_UNIVERSE);

  ui->spinShort->setMaximum(800);
  ui->spinShort->setValue(m_sourceTableModel->shortInterval());
  connect(ui->spinShort, QOverload<int>::of(&QSpinBox::valueChanged), m_sourceTableModel, &SACNSourceTableModel::setShortInterval);

  ui->spinLong->setMaximum(ui->spinShort->maximum());
  ui->spinLong->setValue(m_sourceTableModel->longInterval());
  connect(ui->spinLong, QOverload<int>::of(&QSpinBox::valueChanged), m_sourceTableModel, &SACNSourceTableModel::setLongInterval);

  ui->spinStatic->setMaximum(ui->spinShort->maximum());
  ui->spinStatic->setValue(m_sourceTableModel->staticInterval());
  connect(ui->spinStatic, QOverload<int>::of(&QSpinBox::valueChanged), m_sourceTableModel, &SACNSourceTableModel::setStaticInterval);

  // Maybe don't show the Secure column
  ui->sourceTableView->setColumnHidden(SACNSourceTableModel::COL_PATHWAY_SECURE, !Preferences::Instance().GetPathwaySecureRx());
}

MultiView::~MultiView()
{
  delete ui;
}

void MultiView::on_btnStartStop_clicked(bool checked)
{
  if (checked)
  {
    ui->btnStartStop->setText(tr("Stop"));
    ui->spinUniverseMin->setEnabled(false);
    ui->spinUniverseMax->setEnabled(false);

    // Clear and restart listening for the large number of universes
    m_sourceTableModel->clear();

    // Hold onto the old listeners so any overlaps will not be destructed
    std::map<uint16_t, sACNManager::tListener> old_listeners;
    old_listeners.swap(m_listeners);

    const uint16_t minUniverse = static_cast<uint16_t>(ui->spinUniverseMin->value());
    const uint16_t maxUniverse = static_cast<uint16_t>(ui->spinUniverseMax->value()) + 1;
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
    ui->btnStartStop->setText(tr("Start"));
    ui->spinUniverseMin->setEnabled(true);
    ui->spinUniverseMax->setEnabled(true);
    m_sourceTableModel->pause();
  }
}

void MultiView::on_btnClearOffline_clicked()
{
  if (m_sourceTableModel)
    m_sourceTableModel->clearOffline();
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
