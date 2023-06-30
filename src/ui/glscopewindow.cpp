// Copyright 2023 Electronic Theatre Controls, Inc. or its affiliates
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

#include "glscopewindow.h"

#include "widgets/glscopewidget.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QSplitter>

#include <QComboBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QTableWidget>
#include <QSortFilterProxyModel>

#include <QFileDialog>

GlScopeWindow::GlScopeWindow(int universe, QWidget* parent)
  : QWidget(parent)
  , m_defaultUniverse(universe)
{
  setWindowTitle(tr("Scope"));
  setWindowIcon(QIcon(QStringLiteral(":/icons/scope.png")));

  QBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(2, 2, 2, 2);
  m_splitter = new QSplitter(Qt::Vertical, this);
  layout->addWidget(m_splitter);

  // The oscilloscope view
  QWidget* scopeWidget = new QWidget(this);
  {
    QBoxLayout* layout = new QVBoxLayout(scopeWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);

    m_scope = new GlScopeWidget(scopeWidget);
    connect(m_scope->model(), &ScopeModel::runningChanged, this, &GlScopeWindow::onRunningChanged);
    layout->addWidget(m_scope);

    m_scrollTime = new QScrollBar(Qt::Horizontal, scopeWidget);
    connect(m_scrollTime, &QScrollBar::sliderMoved, this, &GlScopeWindow::onTimeSliderMoved);
    layout->addWidget(m_scrollTime);
    m_disableWhenRunning.push_back(m_scrollTime);
  }
  m_splitter->addWidget(scopeWidget);

  QWidget* confWidget = new QWidget(this);
  {
    QBoxLayout* layoutConf = new QHBoxLayout(confWidget);
    {
      QLabel* lbl = nullptr;
      QGroupBox* grpScope = new QGroupBox(tr("Scope"), confWidget);
      QGridLayout* layoutGrp = new QGridLayout(grpScope);

      int row = 0;
      m_btnStop = new QPushButton(tr("Stop"), confWidget);
      m_btnStop->setCheckable(true);
      connect(m_btnStop, &QPushButton::clicked, m_scope->model(), &ScopeModel::stop);
      layoutGrp->addWidget(m_btnStop, row, 0);

      m_btnStart = new QPushButton(tr("Start"), confWidget);
      m_btnStart->setCheckable(true);
      connect(m_btnStart, &QPushButton::clicked, m_scope->model(), &ScopeModel::start);
      layoutGrp->addWidget(m_btnStart, row, 1);

      ++row;

      lbl = new QLabel(tr("Store:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      QComboBox* recordMode = new QComboBox(confWidget);
      recordMode->addItems({ tr("All Packets"), tr("Level Changes") });
      connect(recordMode, QOverload<int>::of(&QComboBox::activated), this, &GlScopeWindow::setRecordMode);
      recordMode->setCurrentIndex(m_scope->model()->storeAllPoints() ? 0 : 1);
      layoutGrp->addWidget(recordMode, row, 1);

      m_disableWhenRunning.push_back(lbl);
      m_disableWhenRunning.push_back(recordMode);

      ++row;

      lbl = new QLabel(tr("Run For:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      m_spinRunTime = new QSpinBox(confWidget);
      m_spinRunTime->setRange(0, 300); // Five minutes
      m_spinRunTime->setSingleStep(10);
      //! Seconds suffix
      m_spinRunTime->setSuffix(tr("s"));
      m_spinRunTime->setSpecialValueText(tr("Forever"));
      m_spinRunTime->setValue(m_scope->model()->runTime());
      connect(m_spinRunTime, QOverload<int>::of(&QSpinBox::valueChanged), m_scope->model(), &ScopeModel::setRunTime);
      connect(m_scope->model(), &ScopeModel::runTimeChanged, m_spinRunTime, &QSpinBox::setValue);
      layoutGrp->addWidget(m_spinRunTime, row, 1);

      m_disableWhenRunning.push_back(lbl);
      m_disableWhenRunning.push_back(m_spinRunTime);

      ++row;

      // Scope scale configuration
      lbl = new QLabel(tr("Vertical Scale:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      QComboBox* verticalScale = new QComboBox(confWidget);
      verticalScale->addItems({ tr("Percent"), tr("DMX8"), tr("DMX16") });
      connect(verticalScale, QOverload<int>::of(&QComboBox::activated), this, &GlScopeWindow::setVerticalScaleMode);
      verticalScale->setCurrentIndex(static_cast<int>(m_scope->verticalScaleMode()));
      layoutGrp->addWidget(verticalScale, row, 1);

      ++row;

      lbl = new QLabel(tr("Time Scale:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);

      m_spinTimeScale = new QSpinBox(confWidget);
      m_spinTimeScale->setRange(5, 2000); // Milliseconds
      m_spinTimeScale->setSingleStep(5);
      //! Milliseconds suffix
      m_spinTimeScale->setSuffix(tr("ms"));
      m_spinTimeScale->setValue(m_scope->timeDivisions());
      connect(m_spinTimeScale, QOverload<int>::of(&QSpinBox::valueChanged), this, &GlScopeWindow::onTimeDivisionsChanged);
      connect(m_scope, &GlScopeWidget::timeDivisionsChanged, m_spinTimeScale, &QSpinBox::setValue);
      layoutGrp->addWidget(m_spinTimeScale, row, 1);

      // Divider
      ++row;
      QFrame* line = new QFrame(confWidget);
      line->setFrameShape(QFrame::HLine);
      layoutGrp->addWidget(line, row, 0, 1, 2);

      ++row;
      // Trigger setup
      m_triggerType = new QComboBox(confWidget);
      m_triggerType->addItems({
        //! No trigger, runs immediately
        tr("Free Run"),
        //! Triggers when above the target level
        tr("Above"),
        //! Triggers when below the target level
        tr("Below"),
        //! Triggers when passes through or leaves the target level
        tr("Crossed Level") });
      connect(m_triggerType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GlScopeWindow::setTriggerType);
      layoutGrp->addWidget(m_triggerType, row, 0, 1, 2);

      m_disableWhenRunning.push_back(m_triggerType);

      ++row;
      lbl = new QLabel(tr("Trigger Level:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      m_spinTriggerLevel = new QSpinBox(this);
      m_spinTriggerLevel->setRange(0, 65535);
      connect(m_spinTriggerLevel, QOverload<int>::of(&QSpinBox::valueChanged), m_scope->model(), &ScopeModel::setTriggerLevel);
      layoutGrp->addWidget(m_spinTriggerLevel, row, 1);

      m_triggerSetup.push_back(lbl);
      m_triggerSetup.push_back(m_spinTriggerLevel);

      // Set initial trigger type and values
      m_triggerType->setCurrentIndex(0);
      m_spinTriggerLevel->setValue(127);

      // Spacer at the bottom
      ++row;
      layoutGrp->setRowStretch(row, 1);

      layoutConf->addWidget(grpScope);
    }

    {
      QGroupBox* grpChans = new QGroupBox(tr("Channels"), confWidget);
      QBoxLayout* layoutGrp = new QVBoxLayout(grpChans);

      m_tableView = new QTableView(this);
      m_tableView->verticalHeader()->hide();
      m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
      m_tableView->setAlternatingRowColors(true);
      m_tableView->setItemDelegateForColumn(ScopeModel::COL_COLOUR, new ColorPickerDelegate(this));
      m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      // Sorting
      QSortFilterProxyModel* sortProxy = new QSortFilterProxyModel(m_tableView);
      sortProxy->setSourceModel(m_scope->model());
      sortProxy->setSortRole(ScopeModel::DataSortRole);

      m_tableView->setModel(sortProxy);
      m_tableView->setSortingEnabled(true);
      m_tableView->sortByColumn(0, Qt::AscendingOrder);
      m_tableView->horizontalHeader()->setStretchLastSection(true);
      m_tableView->resizeColumnsToContents();

      layoutGrp->addWidget(m_tableView);

      {
        QBoxLayout* layoutBtns = new QHBoxLayout();

        QPushButton* addChan = new QPushButton(tr("Add"), confWidget);
        connect(addChan, &QPushButton::clicked, this, &GlScopeWindow::addTrace);
        layoutBtns->addWidget(addChan);
        m_disableWhenRunning.push_back(addChan);

        QPushButton* removeChan = new QPushButton(tr("Remove"), confWidget);
        connect(removeChan, &QPushButton::clicked, this, &GlScopeWindow::removeTrace);
        layoutBtns->addWidget(removeChan);
        m_disableWhenRunning.push_back(removeChan);

        QPushButton* removeAllChan = new QPushButton(tr("Remove All"), confWidget);
        connect(removeAllChan, &QPushButton::clicked, this, &GlScopeWindow::removeAllTraces);
        layoutBtns->addWidget(removeAllChan);
        m_disableWhenRunning.push_back(removeAllChan);

        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::VLine);
        layoutBtns->addWidget(line);

        QPushButton* btnSave = new QPushButton(tr("Save"), confWidget);
        connect(btnSave, &QPushButton::clicked, this, &GlScopeWindow::saveTraces);
        layoutBtns->addWidget(btnSave);
        m_disableWhenRunning.push_back(btnSave);

        QPushButton* btnLoad = new QPushButton(tr("Load"), confWidget);
        connect(btnLoad, &QPushButton::clicked, this, &GlScopeWindow::loadTraces);
        layoutBtns->addWidget(btnLoad);
        m_disableWhenRunning.push_back(btnLoad);

        layoutBtns->addStretch();

        layoutGrp->addLayout(layoutBtns);
      }

      layoutConf->addWidget(grpChans);
    }
  }

  m_splitter->addWidget(confWidget);

  // Can't collapse the scope
  m_splitter->setCollapsible(0, false);

  // Refresh
  onRunningChanged(m_scope->model()->isRunning());
}

GlScopeWindow::~GlScopeWindow()
{
  // Must delete the scope first
  delete m_scope;
}

void GlScopeWindow::onRunningChanged(bool running)
{
  m_btnStart->setChecked(running);
  m_btnStop->setChecked(!running);

  // Enable/disable the start and stop buttons
  m_btnStart->setEnabled(!running && m_scope->model()->rowCount() > 0);
  m_btnStop->setEnabled(running);

  for (QWidget* w : m_disableWhenRunning)
    w->setEnabled(!running);

  // Disable invalid trigger setup
  for (QWidget* w : m_triggerSetup)
    w->setEnabled(!running && m_triggerType->currentIndex() != static_cast<int>(ScopeModel::Trigger::FreeRun));

  // Reset to start
  if (running)
    m_scope->setScopeView();
  // Force to follow now when running
  m_scope->setFollowNow(running);

  updateTimeScrollBars();
}

void GlScopeWindow::onTimeSliderMoved(int value)
{
  qreal startTime = value;
  startTime = startTime / 1000;
  QRectF scopeView = m_scope->scopeView();
  scopeView.moveLeft(startTime);
  m_scope->setScopeView(scopeView);
}

void GlScopeWindow::onTimeDivisionsChanged(int value)
{
  m_scope->setTimeDivisions(value);
  updateTimeScrollBars();
}

void GlScopeWindow::setRecordMode(int idx)
{
  m_scope->model()->setStoreAllPoints(idx == 0);
}

void GlScopeWindow::setVerticalScaleMode(int idx)
{
  m_scope->setVerticalScaleMode(static_cast<GlScopeWidget::VerticalScale>(idx));
}

void GlScopeWindow::setTriggerType(int idx)
{
  m_scope->model()->setTriggerType(static_cast<ScopeModel::Trigger>(idx));
  // Enable/disable trigger settings
  for (QWidget* w : m_triggerSetup)
    w->setEnabled(idx != static_cast<int>(ScopeModel::Trigger::FreeRun));
}

void GlScopeWindow::addTrace(bool)
{
  QColor traceColor = QColor::fromHsv(m_lastTraceHue, m_lastTraceSat, 255);
  // Shift colour, won't quickly repeat
  m_lastTraceHue += 40;
  if (m_lastTraceHue >= 360)
  {
    m_lastTraceHue -= 360;
    m_lastTraceSat -= 30;
    if (m_lastTraceSat < 0)
      m_lastTraceSat += 255;
  }

  if (m_scope->model()->rowCount() == 0)
  {
    m_scope->model()->addTrace(traceColor, m_defaultUniverse, MIN_DMX_ADDRESS);
    m_btnStart->setEnabled(true);
    return;
  }

  // Add an 8bit trace defaulting to the next item, based on the current index
  QModelIndex current = m_tableView->currentIndex();
  if (!current.isValid())
    current = m_tableView->model()->index(m_tableView->model()->rowCount() - 1, ScopeModel::COL_UNIVERSE);

  uint16_t universe = current.model()->index(current.row(), ScopeModel::COL_UNIVERSE).data(Qt::DisplayRole).toUInt();

  uint16_t address_hi;
  uint16_t address_lo;
  {
    const QString addr_string = current.model()->index(current.row(), ScopeModel::COL_ADDRESS).data(Qt::DisplayRole).toString();
    if (!ScopeTrace::extractAddress(addr_string, address_hi, address_lo))
    {
      qDebug() << "Bad format, logic error:" << addr_string;
      address_hi = 1;
      address_lo = 0;
    }
  }

  if (current.column() == ScopeModel::COL_UNIVERSE)
  {
    // If Universe column is selected, add the same slot(s) in the next unused Universe
    do
    {
      ++universe;
    } while (m_scope->model()->addTrace(traceColor, universe, address_hi, address_lo) == ScopeModel::AddResult::Exists);
  }
  else
  {
    // If any other column is selected, add the next unused 8/16bit slot on the same universe
    if (universe < MIN_SACN_UNIVERSE)
      universe = MIN_SACN_UNIVERSE;
    do
    {
      ++address_hi;

      // 16 bit, default to coarse/fine as is by far the most common
      if (address_lo != 0)
      {
        ++address_hi;
        address_lo = address_hi + 1;
      }

      // Next universe
      if (address_hi > MAX_DMX_ADDRESS)
      {
        ++universe;
        address_hi = MIN_DMX_ADDRESS;
        if (address_lo != 0)
          address_lo = address_hi + 1;
      }
    } while (m_scope->model()->addTrace(traceColor, universe, address_hi, address_lo) == ScopeModel::AddResult::Exists);
  }

  // Select the item that was added
  QAbstractProxyModel* proxy = qobject_cast<QAbstractProxyModel*>(m_tableView->model());
  if (proxy)
  {
    const QModelIndex srcIndex = m_scope->model()->findFirstTraceIndex(universe, address_hi, address_lo, proxy->mapToSource(current).column());
    if (srcIndex.isValid())
      m_tableView->setCurrentIndex(proxy->mapFromSource(srcIndex));
  }
}

void GlScopeWindow::removeTrace(bool)
{
  QItemSelectionModel* selection = m_tableView->selectionModel();
  if (!selection->hasSelection())
    return;

  // Get the items to delete
  QModelIndexList selected = selection->selectedIndexes();
  m_scope->model()->removeTraces(selected);
}

void GlScopeWindow::removeAllTraces(bool)
{
  m_scope->model()->removeAllTraces();
  m_btnStart->setEnabled(false);
}

void GlScopeWindow::saveTraces(bool)
{
  const QString filename = QFileDialog::getSaveFileName(this, tr("Save Traces"), QString(), QStringLiteral("CSV (*.csv)"));
  if (filename.isEmpty())
    return;

  QFile csvExport(filename);
  if (!csvExport.open(QIODevice::WriteOnly))
    return;

  m_scope->model()->saveTraces(csvExport);
}

void GlScopeWindow::loadTraces(bool)
{
  const QString filename = QFileDialog::getOpenFileName(this, tr("Load Traces"), QString(), QStringLiteral("CSV (*.csv)"));
  if (filename.isEmpty())
    return;

  QFile csvExport(filename);
  if (!csvExport.open(QIODevice::ReadOnly))
    return;

  m_scope->model()->loadTraces(csvExport);

  // Update
  updateTimeScrollBars();
  m_btnStart->setEnabled(m_scope->model()->rowCount() > 0);
}

void GlScopeWindow::updateTimeScrollBars()
{
  // Disable scrolling when running
  if (m_scope->model()->isRunning())
  {
    m_scrollTime->setMinimum(0);
    m_scrollTime->setMaximum(0);
    return;
  }

  const QRectF extents = m_scope->model()->traceExtents();
  const qreal viewWidth = m_scope->scopeView().width();
  m_scrollTime->setEnabled(extents.width() > viewWidth);

  // Use milliseconds
  m_scrollTime->setMinimum(extents.left() * 1000);
  const qreal maxVal = (extents.right() - viewWidth) * 1000;
  m_scrollTime->setMaximum(maxVal > 0 ? maxVal : 0);
  m_scrollTime->setPageStep(viewWidth * 1000);

  // And jump to an appropriate end
  if (extents.right() > viewWidth)
    m_scrollTime->setValue(m_scrollTime->maximum());
  else
    m_scrollTime->setValue(m_scrollTime->minimum());

  onTimeSliderMoved(m_scrollTime->value());
  m_scrollTime->setEnabled(true);
}

// QColorDialog doesn't autocentre when used as a delegate
void ColorDialog::showEvent(QShowEvent* ev)
{
  QRect parentRect(parentWidget()->mapToGlobal(QPoint(0, 0)), parentWidget()->size());
  move(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), parentRect).topLeft());

  QColorDialog::showEvent(ev);
}


ColorPickerDelegate::ColorPickerDelegate(QWidget* parent)
  : QStyledItemDelegate(parent)
{
  m_dialog = new ColorDialog(parent);
}

QWidget* ColorPickerDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*idx*/) const
{
  return m_dialog;
}

void ColorPickerDelegate::destroyEditor(QWidget* editor, const QModelIndex& /*idx*/) const
{
  // Don't delete it, reuse it instead
  if (editor == m_dialog)
    return;
}

void ColorPickerDelegate::setEditorData(QWidget* editor, const QModelIndex& idx) const
{
  ColorDialog* dialog = qobject_cast<ColorDialog*>(editor);
  if (dialog)
  {
    QColor color = idx.data(Qt::BackgroundRole).value<QColor>();
    dialog->setCurrentColor(color);
  }
}

void ColorPickerDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& idx) const
{
  ColorDialog* dialog = qobject_cast<ColorDialog*>(editor);
  if (dialog)
  {
    QColor color = dialog->selectedColor();
    if (color.isValid())
      model->setData(idx, color, Qt::EditRole);
  }
}
