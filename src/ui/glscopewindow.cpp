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
#include "widgets/steppedspinbox.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QSplitter>

#include <QCheckBox>
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
    connect(m_scope->model(), &ScopeModel::triggered, this, &GlScopeWindow::onTriggered);
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
      {
        QBoxLayout* startStopBox = new QHBoxLayout();

        m_btnStop = new QPushButton(tr("Stop"), confWidget);
        m_btnStop->setCheckable(true);
        connect(m_btnStop, &QPushButton::clicked, m_scope->model(), &ScopeModel::stop);
        startStopBox->addWidget(m_btnStop);

        m_btnStart = new QPushButton(tr("Start"), confWidget);
        m_btnStart->setCheckable(true);
        connect(m_btnStart, &QPushButton::clicked, m_scope->model(), &ScopeModel::start);
        startStopBox->addWidget(m_btnStart);

        layoutGrp->addLayout(startStopBox, row, 0, 1, 3);
      }

      ++row;
      m_chkSyncViews = new QCheckBox(tr("Trigger Receive Views"), confWidget);
      connect(m_chkSyncViews, &QPushButton::toggled, this, &GlScopeWindow::onChkSyncViewsToggled);
      layoutGrp->addWidget(m_chkSyncViews, row, 0, 1, 3, Qt::AlignHCenter);
      m_disableWhenRunning.push_back(m_chkSyncViews);

      ++row;

      lbl = new QLabel(tr("Store:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      m_recordMode = new QComboBox(confWidget);
      m_recordMode->addItems({ tr("All Packets"), tr("Level Changes") });
      connect(m_recordMode, QOverload<int>::of(&QComboBox::activated), this, &GlScopeWindow::setRecordMode);
      layoutGrp->addWidget(m_recordMode, row, 1, 1, 2);

      m_disableWhenRunning.push_back(lbl);
      m_disableWhenRunning.push_back(m_recordMode);

      ++row;

      lbl = new QLabel(tr("Trace Style:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      m_traceStyle = new QComboBox(confWidget);
      m_traceStyle->addItems({ tr("Line"), tr("Small Dots"), tr("Large Dots") });
      connect(m_traceStyle, QOverload<int>::of(&QComboBox::activated), this, &GlScopeWindow::setTraceStyle);
      layoutGrp->addWidget(m_traceStyle, row, 1, 1, 2);

      ++row;

      lbl = new QLabel(tr("Run For:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      m_spinRunTime = new QSpinBox(confWidget);
      m_spinRunTime->setRange(0, 300); // Five minutes
      m_spinRunTime->setSingleStep(10);
      //! Seconds suffix
      m_spinRunTime->setSuffix(tr("s"));
      m_spinRunTime->setSpecialValueText(tr("Forever"));
      connect(m_spinRunTime, QOverload<int>::of(&QSpinBox::valueChanged), m_scope->model(), &ScopeModel::setRunTime);
      connect(m_scope->model(), &ScopeModel::runTimeChanged, m_spinRunTime, &QSpinBox::setValue);
      layoutGrp->addWidget(m_spinRunTime, row, 1, 1, 2);

      m_disableWhenRunning.push_back(lbl);
      m_disableWhenRunning.push_back(m_spinRunTime);

      ++row;
      //! Scope vertical scale configuration
      lbl = new QLabel(tr("Vertical Scale:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);

      QComboBox* verticalScale = new QComboBox(confWidget);
      verticalScale->addItems({ tr("Percent"), tr("DMX8"), tr("DMX16"), tr("Delta Time") });
      connect(verticalScale, QOverload<int>::of(&QComboBox::activated), this, &GlScopeWindow::setVerticalScaleMode);
      verticalScale->setCurrentIndex(static_cast<int>(m_scope->verticalScaleMode()));
      layoutGrp->addWidget(verticalScale, row, 1);

      m_spinVertScale = new SteppedSpinBox(confWidget);
      m_spinVertScale->setStepList({ 50,100,250,E131_DATA_KEEP_ALIVE_INTERVAL_MAX,E131_NETWORK_DATA_LOSS_TIMEOUT }); // Milliseconds
      //! Milliseconds suffix
      m_spinVertScale->setSuffix(tr("ms"));
      m_spinVertScale->setValue(GlScopeWidget::scopeVerticalMaxDefault(GlScopeWidget::VerticalScale::DeltaTime));
      connect(m_spinVertScale, QOverload<int>::of(&QSpinBox::valueChanged), this, &GlScopeWindow::onVerticalScaleChanged);
      layoutGrp->addWidget(m_spinVertScale, row, 2);

      ++row;

      lbl = new QLabel(tr("Time Scale:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);

      m_timeFormat = new QComboBox(confWidget);
      m_timeFormat->addItems({
        //! Elapsed time
        tr("Elapsed"),
        //! Wallclock time
        tr("Wallclock")
        });

      connect(m_timeFormat, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GlScopeWindow::setTimeFormat);
      layoutGrp->addWidget(m_timeFormat, row, 1);

      m_spinTimeScale = new SteppedSpinBox(confWidget);
      m_spinTimeScale->setStepList({ 5,10,20,50,100,200,500,1000,2000 }); // Milliseconds
      //! Milliseconds suffix
      m_spinTimeScale->setSuffix(tr("ms"));
      m_spinTimeScale->setValue(m_scope->timeDivisions());
      connect(m_spinTimeScale, QOverload<int>::of(&QSpinBox::valueChanged), this, &GlScopeWindow::onTimeDivisionsChanged);
      connect(m_scope, &GlScopeWidget::timeDivisionsChanged, m_spinTimeScale, &QSpinBox::setValue);
      layoutGrp->addWidget(m_spinTimeScale, row, 2);


      // Divider
      ++row;
      QFrame* line = new QFrame(confWidget);
      line->setFrameShape(QFrame::HLine);
      layoutGrp->addWidget(line, row, 0, 1, 3);

      ++row;
      lbl = new QLabel(tr("Trigger:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);

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
        tr("Crossed Level")
        });
      connect(m_triggerType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GlScopeWindow::setTriggerType);
      layoutGrp->addWidget(m_triggerType, row, 1);

      m_disableWhenRunning.push_back(m_triggerType);

      m_spinTriggerLevel = new QSpinBox(this);
      m_spinTriggerLevel->setRange(0, ScopeModel::kMaxDmx16);
      connect(m_spinTriggerLevel, QOverload<int>::of(&QSpinBox::valueChanged), m_scope->model(), &ScopeModel::setTriggerLevel);
      layoutGrp->addWidget(m_spinTriggerLevel, row, 2);

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
  updateConfiguration();
  onRunningChanged(m_scope->model()->isRunning());

  // Now connect signals for other receiver views
  connect(this, SIGNAL(startOtherViews()), parent, SIGNAL(startReceiverViews()));
  connect(this, SIGNAL(stopOtherViews()), parent, SIGNAL(stopReceiverViews()));
}

GlScopeWindow::~GlScopeWindow()
{
  // Must delete the scope first
  delete m_scope;
}

void GlScopeWindow::onRunningChanged(bool running)
{
  if (m_chkSyncViews->isChecked() && !running)
    emit stopOtherViews();

  refreshButtons();

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

void GlScopeWindow::setTimeFormat(int value)
{
  m_scope->setTimeFormat(static_cast<GlScopeWidget::TimeFormat>(value));
}

void GlScopeWindow::setRecordMode(int idx)
{
  m_scope->model()->setStoreAllPoints(idx == 0);
}

void GlScopeWindow::setTraceStyle(int idx)
{
  switch (idx)
  {
  case 0: m_scope->setDotSize(0.0f); return; // Line only
  case 1: m_scope->setDotSize(3.0f); return; // Small dots
  case 2: m_scope->setDotSize(5.0f); return; // Large dots
  }
}

void GlScopeWindow::setVerticalScaleMode(int idx)
{
  GlScopeWidget::VerticalScale scaleMode = static_cast<GlScopeWidget::VerticalScale>(idx);
  m_scope->setVerticalScaleMode(scaleMode);
  if (scaleMode == GlScopeWidget::VerticalScale::DeltaTime)
  {
    m_spinVertScale->setEnabled(true);
    // Reapply the current setting
    onVerticalScaleChanged(-1);
  }
  else
  {
    m_spinVertScale->setEnabled(false);
  }
}

void GlScopeWindow::onVerticalScaleChanged(int value)
{
  if (value <= 0)
    value = m_spinVertScale->value();

  m_scope->setScopeViewVerticalRange(0, value);
}

void GlScopeWindow::setTriggerType(int idx)
{
  m_scope->model()->setTriggerType(static_cast<ScopeModel::Trigger>(idx));
  refreshButtons();
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
    refreshButtons();
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

  // Get the items to delete via the proxy
  QAbstractProxyModel* proxy = qobject_cast<QAbstractProxyModel*>(m_tableView->model());
  if (proxy)
    m_scope->model()->removeTraces(proxy->mapSelectionToSource(selection->selection()).indexes());
  else
    m_scope->model()->removeTraces(selection->selectedIndexes());
  refreshButtons();
}

void GlScopeWindow::removeAllTraces(bool)
{
  m_scope->model()->removeAllTraces();
  refreshButtons();
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
  updateConfiguration();
  refreshButtons();
}

void GlScopeWindow::onTriggered()
{
  if (m_chkSyncViews->isChecked())
    emit startOtherViews();
}

void GlScopeWindow::onChkSyncViewsToggled(bool /*checked*/)
{
  refreshButtons();
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

void GlScopeWindow::updateConfiguration()
{
  // Read values back from the scope model
  m_recordMode->setCurrentIndex(m_scope->model()->storeAllPoints() ? 0 : 1);
  m_spinRunTime->setValue(m_scope->model()->runTime());
  m_triggerType->setCurrentIndex(static_cast<int>(m_scope->model()->triggerType()));
  m_spinTriggerLevel->setValue(m_scope->model()->triggerLevel());
}

void GlScopeWindow::refreshButtons()
{
  const bool running = m_scope->model()->isRunning();

  m_btnStart->setChecked(running);
  m_btnStop->setChecked(!running);

  // Enable/disable the stop button
  m_btnStop->setEnabled(running);

  for (QWidget* w : m_disableWhenRunning)
    w->setEnabled(!running);

  // Vertical scale is only adjustable in DeltaTime mode
  m_spinVertScale->setEnabled(m_scope->verticalScaleMode() == GlScopeWidget::VerticalScale::DeltaTime);

  // Disable invalid trigger setup
  for (QWidget* w : m_triggerSetup)
    w->setEnabled(!running && !m_scope->model()->triggerIsFreeRun());

  if (!running)
  {
    // Can start if any traces exist
    if (m_scope->model()->rowCount() > 0)
    {
      m_btnStart->setEnabled(true);
      return;
    }

    // Can also run on empty if Trigger type is Free Run and synced
    if (m_chkSyncViews->isChecked() && m_scope->model()->triggerIsFreeRun())
    {
      m_btnStart->setEnabled(true);
      return;
    }
  }

  // Cannot start. Either running or nothing to do
  m_btnStart->setEnabled(false);
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
