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

#include <QFileDialog>

GlScopeWindow::GlScopeWindow(QWidget* parent)
  : QWidget(parent)
{
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
  }
  m_splitter->addWidget(scopeWidget);

  QWidget* confWidget = new QWidget(this);
  {
    QBoxLayout* layoutConf = new QHBoxLayout(confWidget);
    {
      QGroupBox* grpScope = new QGroupBox(tr("Scope"), confWidget);
      QGridLayout* layoutGrp = new QGridLayout(grpScope);

      int row = 0;
      m_btnStart = new QPushButton(tr("Start"), confWidget);
      m_btnStart->setCheckable(true);
      connect(m_btnStart, &QPushButton::clicked, m_scope->model(), &ScopeModel::start);
      layoutGrp->addWidget(m_btnStart, row, 0);

      m_btnStop = new QPushButton(tr("Stop"), confWidget);
      m_btnStop->setCheckable(true);
      connect(m_btnStop, &QPushButton::clicked, m_scope->model(), &ScopeModel::stop);
      layoutGrp->addWidget(m_btnStop, row, 1);

      ++row;

      // Scope scale configuration
      QLabel* lbl = new QLabel(tr("Vertical Scale:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      QComboBox* verticalScale = new QComboBox(confWidget);
      verticalScale->addItems({ tr("DMX"), tr("Percent") });
      connect(verticalScale, QOverload<int>::of(&QComboBox::activated), this, &GlScopeWindow::setVerticalScaleMode);
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
      connect(m_spinTimeScale, QOverload<int>::of(&QSpinBox::valueChanged), m_scope, &GlScopeWidget::setTimeDivisions);
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
      m_triggerType->addItems({ tr("Free Run"), tr("Rising Edge"), tr("Falling Edge") });
      connect(m_triggerType, QOverload<int>::of(&QComboBox::activated), this, &GlScopeWindow::setTriggerType);
      layoutGrp->addWidget(m_triggerType, row, 0, 1, 2);

      ++row;
      lbl = new QLabel(tr("Trigger Level:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      m_spinTriggerLevel = new QSpinBox(this);
      m_spinTriggerLevel->setRange(0, 65535);
      connect(m_spinTriggerLevel, QOverload<int>::of(&QSpinBox::valueChanged), m_scope->model(), &ScopeModel::setTriggerLevel);
      layoutGrp->addWidget(m_spinTriggerLevel, row, 1);

      ++row;
      lbl = new QLabel(tr("Trigger Delay:"), confWidget);
      layoutGrp->addWidget(lbl, row, 0);
      m_spinTriggerDelay = new QSpinBox(this);
      m_spinTriggerDelay->setRange(0, 1000);  // Milliseconds
      //! Milliseconds suffix
      m_spinTriggerDelay->setSuffix(tr("ms"));
      connect(m_spinTriggerDelay, QOverload<int>::of(&QSpinBox::valueChanged), m_scope->model(), &ScopeModel::setTriggerDelay);
      layoutGrp->addWidget(m_spinTriggerDelay, row, 1);

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
      m_tableView->setModel(m_scope->model());

      layoutGrp->addWidget(m_tableView);

      {
        QBoxLayout* layoutBtns = new QHBoxLayout();

        QPushButton* addChan = new QPushButton(tr("Add"), confWidget);
        layoutBtns->addWidget(addChan);
        QPushButton* removeChan = new QPushButton(tr("Remove"), confWidget);
        layoutBtns->addWidget(removeChan);

        QFrame* line = new QFrame(this);
        line->setFrameShape(QFrame::VLine);
        layoutBtns->addWidget(line);

        QPushButton* btnSave = new QPushButton(tr("Save"), confWidget);
        connect(btnSave, &QPushButton::clicked, this, &GlScopeWindow::saveTraces);
        layoutBtns->addWidget(btnSave);
        QPushButton* btnLoad = new QPushButton(tr("Load"), confWidget);
        connect(btnLoad, &QPushButton::clicked, this, &GlScopeWindow::loadTraces);
        layoutBtns->addWidget(btnLoad);

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

  // Enable/Disable trigger items
  m_triggerType->setEnabled(!running);
  m_spinTriggerLevel->setEnabled(!running);
  m_spinTriggerDelay->setEnabled(!running);

  // Force to follow now when running
  m_scope->setFollowNow(running);

  updateScrollBars();
}

void GlScopeWindow::setVerticalScaleMode(int idx)
{
  m_scope->setVerticalScaleMode(static_cast<GlScopeWidget::VerticalScale>(idx + 1));
}

void GlScopeWindow::setTriggerType(int idx)
{
  m_scope->model()->setTriggerType(static_cast<ScopeModel::Trigger>(idx + 1));
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
  updateScrollBars();
}

void GlScopeWindow::onTimeSliderMoved(int value)
{
  qreal startTime = value;
  startTime = startTime / 1000;
  QRectF scopeView = m_scope->scopeView();
  scopeView.moveLeft(startTime);
  m_scope->setScopeView(scopeView);
}

void GlScopeWindow::updateScrollBars()
{
  // Disable scrolling when running
  if (m_scope->model()->isRunning())
  {
    m_scrollTime->setEnabled(false);
    m_scrollTime->setMinimum(0);
    m_scrollTime->setMaximum(0);
    return;
  }

  const QRectF extents = m_scope->model()->traceExtents();
  const qreal viewWidth = m_scope->scopeView().width();
  m_scrollTime->setEnabled(extents.width() > viewWidth);

  // Use milliseconds
  m_scrollTime->setMinimum(extents.left() * 1000);
  m_scrollTime->setMaximum((extents.right() - viewWidth) * 1000);
  m_scrollTime->setPageStep(viewWidth * 1000);
  m_scrollTime->setValue(m_scope->scopeView().left() * 1000);

  m_scrollTime->setEnabled(true);
}
