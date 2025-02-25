// Copyright 2016 Tom Steer
// http://www.tomsteer.net
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

#include "mdimainwindow.h"
#include "ui_mdimainwindow.h"
#include "glscopewindow.h"
#include "universeview.h"
#include "transmitwindow.h"
#include "preferences.h"
#include "preferencesdialog.h"
#include "aboutdialog.h"
#include "sacnuniverselistmodel.h"
#include "sacndiscoveredsourcelistmodel.h"
#include "sacnsynclistmodel.h"
#include "snapshot.h"
#include "multiuniverse.h"
#include "multiview.h"
#include "pcapplayback.h"

#include <QMdiArea>
#include <QMdiSubWindow>

static constexpr int kDockStateVersion = 1; // Increment whenever changing Docking

MDIMainWindow::MDIMainWindow(QWidget* parent) :
  QMainWindow(parent),
  ui(new Ui::MDIMainWindow),
  m_model(new sACNUniverseListModel(this)),
  m_modelDiscovered(new sACNDiscoveredSourceListModel(this)),
  m_proxyDiscovered(new sACNDiscoveredSourceListProxy(this)),
  m_modelSync(new sACNSyncListModel(this)),
  m_proxySync(new sACNSyncListModel::proxy(this))
{
  ui->setupUi(this);
}

MDIMainWindow::~MDIMainWindow()
{
  qDeleteAll(m_subWindows);
  delete ui;
}

void MDIMainWindow::showEvent(QShowEvent* ev)
{
  if (Preferences::Instance().GetWindowMode() == WindowMode::Floating && !Preferences::Instance().GetRestoreWindowLayout())
    resize(310, 560);

  QMainWindow::showEvent(ev);

  // Universe list
  ui->treeView->setModel(m_model);
  ui->treeView->expandAll();
  connect(ui->treeView, &QAbstractItemView::doubleClicked, this, &MDIMainWindow::universeDoubleClick);
  connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved, this, &MDIMainWindow::rowsAboutToBeRemoved);

  ui->sbUniverseList->setMinimum(MIN_SACN_UNIVERSE);
  ui->sbUniverseList->setMaximum(MAX_SACN_UNIVERSE - Preferences::Instance().GetUniversesListed() + 1);
  ui->sbUniverseList->setWrapping(true);
  ui->sbUniverseList->setValue(MIN_SACN_UNIVERSE);

  // Discovered sources list
  m_proxyDiscovered->setSourceModel(m_modelDiscovered);
  ui->treeViewDiscovered->setModel(m_proxyDiscovered);
  connect(ui->treeViewDiscovered, &QAbstractItemView::doubleClicked, this, &MDIMainWindow::universeDoubleClick);
  ui->treeViewDiscovered->setSortingEnabled(true);
  ui->treeViewDiscovered->sortByColumn(0, Qt::AscendingOrder);

  // Universe Synchronization details
  m_proxySync->setSourceModel(m_modelSync);
  ui->treeViewSync->setModel(m_proxySync);
  connect(ui->treeViewSync, &QAbstractItemView::doubleClicked, this, &MDIMainWindow::universeDoubleClick);
  ui->treeViewSync->setSortingEnabled(true);
  ui->treeViewSync->sortByColumn(0, Qt::AscendingOrder);

  // And apply prefs
  applyPrefs();
}

void MDIMainWindow::closeEvent(QCloseEvent* ev)
{
  if (Preferences::Instance().GetAutoSaveWindowLayout())
    saveSubWindows();

  qDeleteAll(m_subWindows);
  m_subWindows.clear();

  QMainWindow::closeEvent(ev);
}

void MDIMainWindow::on_actionScopeView_triggered(bool checked)
{
  Q_UNUSED(checked);
  GlScopeWindow* scopeWindow = new GlScopeWindow(ui->sbUniverseList->value(), this);
  showWidgetAsSubWindow(scopeWindow);
}

void MDIMainWindow::on_actionRecieve_triggered(bool checked)
{
  Q_UNUSED(checked);
  UniverseView* uniView = new UniverseView(getSelectedUniverse(), this);
  showWidgetAsSubWindow(uniView);
}

void MDIMainWindow::on_actionMultiView_triggered(bool checked)
{
  Q_UNUSED(checked);
  MultiView* multiView = new MultiView(this);
  showWidgetAsSubWindow(multiView);
}

void MDIMainWindow::on_actionTranmsit_triggered(bool checked)
{
  Q_UNUSED(checked);
  transmitwindow* trView = new transmitwindow(getSelectedUniverse(), this);
  showWidgetAsSubWindow(trView);
}

void MDIMainWindow::on_actionSnapshot_triggered(bool checked)
{
  Q_UNUSED(checked);
  Snapshot* snapView = new Snapshot(ui->sbUniverseList->value(), this);
  showWidgetAsSubWindow(snapView);
}

void MDIMainWindow::on_actionSettings_triggered(bool /*checked*/)
{
  if (m_prefDialog == nullptr)
  {
    m_prefDialog = new PreferencesDialog(this);
    connect(m_prefDialog, &QDialog::accepted, this, &MDIMainWindow::applyPrefs);
    connect(m_prefDialog, &PreferencesDialog::storeWindowLayoutNow, this, &MDIMainWindow::saveSubWindows);
  }

  m_prefDialog->open();
}

void MDIMainWindow::on_actionAbout_triggered(bool checked)
{
  Q_UNUSED(checked);
  aboutDialog* about = new aboutDialog(this);
  about->exec();
}

void MDIMainWindow::on_btnUnivListBack_pressed()
{
  ui->sbUniverseList->setValue(ui->sbUniverseList->value() - Preferences::Instance().GetUniversesListed());
}

void MDIMainWindow::on_btnUnivListForward_pressed()
{

  ui->sbUniverseList->setValue(ui->sbUniverseList->value() + Preferences::Instance().GetUniversesListed());
}

void MDIMainWindow::on_sbUniverseList_valueChanged(int value)
{
  if (m_model)
    m_model->setStartUniverse(value);
}


void MDIMainWindow::universeDoubleClick(const QModelIndex& index)
{
  int universe = 0;

  if (ui->treeView->isVisible())
  {
    if (!m_model) return;
    universe = m_model->indexToUniverse(index);
  }
  else if (ui->treeViewDiscovered->isVisible())
  {
    if (!m_modelDiscovered) return;
    QModelIndex srcIndex = m_proxyDiscovered->mapToSource(index);
    universe = m_modelDiscovered->indexToUniverse(srcIndex);
  }
  else if (ui->treeViewSync->isVisible())
  {
    if (!m_modelSync) return;
    universe = m_modelSync->indexToUniverse(index);
  }

  if (universe > 0)
  {
    UniverseView* uniView = new UniverseView(universe, this);
    showWidgetAsSubWindow(uniView);
    uniView->startRx();
  }

}

void MDIMainWindow::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
  QModelIndex index = ui->treeView->currentIndex();
  if ((parent.model() == index.model()) && (index.row() >= start) && (index.row() <= end)) {
    ui->treeView->clearSelection();
  }
}

void MDIMainWindow::on_actionMultiUniverse_triggered()
{
  MultiUniverse* multiUniv = new MultiUniverse(ui->sbUniverseList->value(), this);
  showWidgetAsSubWindow(multiUniv);
}

QWidget* MDIMainWindow::showWidgetAsSubWindow(QWidget* w)
{
  switch (Preferences::Instance().GetWindowMode())
  {
  default:
  case WindowMode::MDI: return addMdiWidget(w);
  case WindowMode::Floating: return addFloatWidget(w);
  }
}

void MDIMainWindow::saveSubWindows()
{
  Preferences& p = Preferences::Instance();
  p.SetMainWindowGeometry(saveGeometry(), saveState(kDockStateVersion));

  QList<SubWindowInfo> result;
  if (m_mdiArea)
  {
    QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList();
    for (const QMdiSubWindow* window : windows)
    {
      StoreWidgetGeometry(window, window->widget(), result);
    }
  }
  else
  {
    for (const QWidget* window : m_subWindows)
    {
      StoreWidgetGeometry(window, window, result);
    }
  }

  p.SetSavedWindows(result);
}

void MDIMainWindow::restoreSubWindows()
{
  const Preferences& p = Preferences::Instance();
  if (!p.GetRestoreWindowLayout())
    return;

  restoreGeometry(p.GetMainWindowGeometry());
  restoreState(p.GetMainWindowState(), kDockStateVersion);

  const QList<SubWindowInfo>& windows = p.GetSavedWindows();
  for (const SubWindowInfo& window : windows)
  {
    QWidget* widget = Q_NULLPTR;

    // Construct supported windows
    if (window.name == "Scope")
    {
      widget = new GlScopeWindow(MIN_SACN_UNIVERSE, this);
    }
    else if (window.name == "Universe")
    {
      widget = new UniverseView(MIN_SACN_UNIVERSE, this);
    }
    else if (window.name == "Transmit")
    {
      widget = new transmitwindow(MIN_SACN_UNIVERSE, this);
    }
    else if (window.name == "Snapshot")
    {
      widget = new Snapshot(MIN_SACN_UNIVERSE, this);
    }
    else if (window.name == "MultiUniverse")
    {
      widget = new MultiUniverse(MIN_SACN_UNIVERSE, this);
    }
    else if (window.name == "MultiView")
    {
      widget = new MultiView(MIN_SACN_UNIVERSE, this);
    }

    if (widget)
    {
      showWidgetAsSubWindow(widget)->restoreGeometry(window.geometry);
      // Attempt to set the config
      if (!window.config.isEmpty())
        QMetaObject::invokeMethod(widget, "setJsonConfiguration", Q_ARG(QJsonObject, window.config));
    }

  }

  if (p.GetAutoStartRX())
  {
    // Automatically start all receiver views after a moment
    QTimer::singleShot(500, this, &MDIMainWindow::startReceiverViews);
  }
}

void MDIMainWindow::startReceiverViews()
{
  // Invoke startRx() on everything except the sender
  for (QWidget *widget : m_subWindows)
  {
    if (sender() == widget)
      continue;
    QMetaObject::invokeMethod(widget, "startRx");
  }
}

void MDIMainWindow::stopReceiverViews()
{
  // Invoke stopRx() on everything except the sender
  for (QWidget* widget : m_subWindows)
  {
    if (sender() == widget)
      continue;
    QMetaObject::invokeMethod(widget, "stopRx");
  }
}

void MDIMainWindow::on_actionPCAPPlayback_triggered()
{
  PcapPlayback* pcapPlayback = new PcapPlayback(this);
  showWidgetAsSubWindow(pcapPlayback);
}

int MDIMainWindow::getSelectedUniverse()
{
  QModelIndex selectedIndex = ui->treeView->currentIndex();
  int selectedUniverse = m_model->indexToUniverse(selectedIndex);
  return (selectedUniverse >= MIN_SACN_UNIVERSE && selectedUniverse <= MAX_SACN_UNIVERSE) ? selectedUniverse : 1;
}

QWidget* MDIMainWindow::addMdiWidget(QWidget* w)
{
  if (!m_mdiArea)
  {
    m_mdiArea = new QMdiArea();
    m_mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QWidget* oldCentral = centralWidget();
    setCentralWidget(m_mdiArea);
    if (oldCentral)
    {
      qDebug() << "Destroying" << oldCentral->objectName();
      delete oldCentral;
    }
  }

  // Parent it to an MDI subwindow
  w->setAttribute(Qt::WA_DeleteOnClose, false); // Let the MDI area control lifetime
  QWidget* result = m_mdiArea->addSubWindow(w);
  w->show();
  return result;
}

QWidget* MDIMainWindow::addFloatWidget(QWidget* w)
{
  // Promote to a toplevel window but keep track of it
  if (!m_subWindows.contains(w))
  {
    m_subWindows.append(w);
    connect(w, &QObject::destroyed, this, &MDIMainWindow::subWindowRemoved);
  }

  w->setParent(nullptr);
  w->setAttribute(Qt::WA_DeleteOnClose);
  w->show();
  return w;
}

void MDIMainWindow::StoreWidgetGeometry(const QWidget* window, const QWidget* widget, QList<SubWindowInfo>& result)
{
  SubWindowInfo i;

  if (qobject_cast<const GlScopeWindow*>(widget) != Q_NULLPTR)
  {
    i.name = "Scope";
  }
  else if (qobject_cast<const UniverseView*>(widget) != Q_NULLPTR)
  {
    i.name = "Universe";
  }
  else if (qobject_cast<const transmitwindow*>(widget) != Q_NULLPTR)
  {
    i.name = "Transmit";
  }
  else if (qobject_cast<const Snapshot*>(widget) != Q_NULLPTR)
  {
    i.name = "Snapshot";
  }
  else if (qobject_cast<const MultiUniverse*>(widget) != Q_NULLPTR)
  {
    i.name = "MultiUniverse";
  }
  else if (qobject_cast<const MultiView*>(widget) != Q_NULLPTR)
  {
    i.name = "MultiView";
  }

  // Add to list if supported
  if (!i.name.isEmpty())
  {
    i.geometry = window->saveGeometry();
    QMetaObject::invokeMethod(const_cast<QWidget*>(widget), "getJsonConfiguration", Qt::DirectConnection, Q_RETURN_ARG(QJsonObject, i.config));
    result.append(i);
  }
}

void MDIMainWindow::on_pbFewer_clicked()
{
  Preferences::Instance().SetUniversesListed(Preferences::Instance().GetUniversesListed() - 1);
  on_sbUniverseList_valueChanged(ui->sbUniverseList->value());
}

void MDIMainWindow::on_pbMore_clicked()
{
  Preferences::Instance().SetUniversesListed(Preferences::Instance().GetUniversesListed() + 1);
  on_sbUniverseList_valueChanged(ui->sbUniverseList->value());
}

void MDIMainWindow::subWindowRemoved()
{
  QWidget* widget = qobject_cast<QWidget*>(sender());
  if (widget)
    m_subWindows.removeOne(widget);
}

void MDIMainWindow::applyPrefs()
{
  // Apply new window mode
  if (Preferences::Instance().GetWindowMode() != m_currentWindowMode)
  {
    qDebug("Changing windowing mode");
    switch (Preferences::Instance().GetWindowMode())
    {
    case WindowMode::COUNT: Preferences::Instance().SetWindowMode(WindowMode::MDI);
      [[fallthrough]];

    case WindowMode::MDI:
    {
      // Move the Universe List widget back into the DockWidget if necessary
      if (centralWidget() == ui->widgetUniverseList)
      {
        ui->dockWidgetContents->layout()->addWidget(ui->widgetUniverseList);
      }
      ui->dwUnivList->show();
      // Reparent all floating windows to the MDI Area
      for (QWidget* widget : m_subWindows)
      {
        disconnect(widget, &QObject::destroyed, this, &MDIMainWindow::subWindowRemoved);
        addMdiWidget(widget);
      }
      m_subWindows.clear();
    } break;

    case WindowMode::Floating:
    {
      if (m_mdiArea)
      {
        // Promote all MDI widgets to top-level
        QList<QMdiSubWindow*> windows = m_mdiArea->subWindowList(QMdiArea::StackingOrder);
        for (QMdiSubWindow* window : windows)
        {
          addFloatWidget(window->widget());
          delete window;
        }
        delete m_mdiArea;
        m_mdiArea = nullptr;
      }

      // Move the Universe List dockwidget to the centralWidget
      setCentralWidget(ui->widgetUniverseList);
      ui->dwUnivList->hide();
    } break;
    }
    // Done, store mode
    m_currentWindowMode = Preferences::Instance().GetWindowMode();
  }
}
