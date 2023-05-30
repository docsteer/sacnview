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
#include "scopewindow.h"
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

#include <QMdiSubWindow>


MDIMainWindow::MDIMainWindow(QWidget *parent) :
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
    delete ui;
}

void MDIMainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);

    // Universe list
    ui->treeView->setModel(m_model);
    ui->treeView->expandAll();
    connect(ui->treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(universeDoubleClick(QModelIndex)));
    connect(m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));

    ui->sbUniverseList->setMinimum(MIN_SACN_UNIVERSE);
    ui->sbUniverseList->setMaximum(MAX_SACN_UNIVERSE - Preferences::Instance().GetUniversesListed() + 1);
    ui->sbUniverseList->setWrapping(true);
    ui->sbUniverseList->setValue(MIN_SACN_UNIVERSE);

    // Discovered sources list
    m_proxyDiscovered->setSourceModel(m_modelDiscovered);
    ui->treeViewDiscovered->setModel(m_proxyDiscovered);
    connect(ui->treeViewDiscovered, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(universeDoubleClick(QModelIndex)));
    ui->treeViewDiscovered->setSortingEnabled(true);
    ui->treeViewDiscovered->sortByColumn(0, Qt::AscendingOrder);

    // Universe Synchronization details
    m_proxySync->setSourceModel(m_modelSync);
    ui->treeViewSync->setModel(m_proxySync);
    connect(ui->treeViewSync, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(universeDoubleClick(QModelIndex)));
    ui->treeViewSync->setSortingEnabled(true);
    ui->treeViewSync->sortByColumn(0, Qt::AscendingOrder);
}

void MDIMainWindow::on_actionScopeView_triggered(bool checked)
{
    Q_UNUSED(checked);
    ScopeWindow *scopeWindow = new ScopeWindow(ui->sbUniverseList->value(), this);
    ui->mdiArea->addSubWindow(scopeWindow);
    scopeWindow->show();
}

void MDIMainWindow::on_actionRecieve_triggered(bool checked)
{
    Q_UNUSED(checked);
    UniverseView *uniView = new UniverseView(getSelectedUniverse(), this);
    ui->mdiArea->addSubWindow(uniView);
    uniView->show();
}

void MDIMainWindow::on_actionMultiView_triggered(bool checked)
{
    Q_UNUSED(checked);
    MultiView* multiView = new MultiView(this);
    ui->mdiArea->addSubWindow(multiView);
    multiView->show();
}

void MDIMainWindow::on_actionTranmsit_triggered(bool checked)
{
    Q_UNUSED(checked);
    transmitwindow *trView = new transmitwindow(getSelectedUniverse(), this);
    ui->mdiArea->addSubWindow(trView);
    trView->show();
}

void MDIMainWindow::on_actionSnapshot_triggered(bool checked)
{
    Q_UNUSED(checked);
    Snapshot *snapView = new Snapshot(ui->sbUniverseList->value(), this);
    ui->mdiArea->addSubWindow(snapView);
    snapView->show();
}

void MDIMainWindow::on_actionSettings_triggered(bool checked)
{
    Q_UNUSED(checked);
    PreferencesDialog *prefDialog = new PreferencesDialog(this);
    prefDialog->exec();

}

void MDIMainWindow::on_actionAbout_triggered(bool checked)
{
    Q_UNUSED(checked);
    aboutDialog *about =new aboutDialog(this);
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
    if(m_model)
        m_model->setStartUniverse(value);
}


void MDIMainWindow::universeDoubleClick(const QModelIndex &index)
{
    int universe = 0;

    if (ui->treeView->isVisible())
    {
        if(!m_model) return;
        universe = m_model->indexToUniverse(index);
    } else if (ui->treeViewDiscovered->isVisible())
    {
        if(!m_modelDiscovered) return;
        QModelIndex srcIndex = m_proxyDiscovered->mapToSource(index);
        universe = m_modelDiscovered->indexToUniverse(srcIndex);
    } else if (ui->treeViewSync->isVisible())
    {
        if(!m_modelSync) return;
        universe = m_modelSync->indexToUniverse(index);
    }

    if (universe>0)
    {
        UniverseView *uniView = new UniverseView(1, this);
        ui->mdiArea->addSubWindow(uniView);
        uniView->show();
        uniView->startListening(universe);
    }

}

void MDIMainWindow::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    QModelIndex index = ui->treeView->currentIndex();
    if ((parent.model() == index.model()) && (index.row() >= start) && (index.row() <= end)) {
        ui->treeView->clearSelection();
    }
}

void MDIMainWindow::on_actionMultiUniverse_triggered()
{
    MultiUniverse *multiUniv = new MultiUniverse(ui->sbUniverseList->value(), this);
    ui->mdiArea->addSubWindow(multiUniv);
    multiUniv->show();
}

void MDIMainWindow::showWidgetAsMdiWindow(QWidget *w)
{
    ui->mdiArea->addSubWindow(w);
    w->show();
}

void MDIMainWindow::saveMdiWindows()
{
    Preferences &p = Preferences::Instance();
    if(p.GetSaveWindowLayout())
    {
        p.SetMainWindowGeometry(saveGeometry());

        QList<MDIWindowInfo> result;
        QList<QMdiSubWindow *> windows = ui->mdiArea->subWindowList();
        foreach(QMdiSubWindow *window, windows)
        {
            if(qobject_cast<ScopeWindow*>(window->widget())!=Q_NULLPTR)
            {
                MDIWindowInfo i;
                i.name = "Scope";
                i.geometry = window->saveGeometry();
                result << i;
            }
            if(qobject_cast<UniverseView*>(window->widget())!=Q_NULLPTR)
            {
                MDIWindowInfo i;
                i.name = "Universe";
                i.geometry = window->saveGeometry();
                result << i;
            }
            if(qobject_cast<transmitwindow*>(window->widget())!=Q_NULLPTR)
            {
                MDIWindowInfo i;
                i.name = "Transmit";
                i.geometry = window->saveGeometry();
                result << i;
            }
            if(qobject_cast<Snapshot*>(window->widget())!=Q_NULLPTR)
            {
                MDIWindowInfo i;
                i.name = "Snapshot";
                i.geometry = window->saveGeometry();
                result << i;
            }
            if(qobject_cast<MultiUniverse*>(window->widget())!=Q_NULLPTR)
            {
                MDIWindowInfo i;
                i.name = "MultiUniverse";
                i.geometry = window->saveGeometry();
                result << i;
            }
        }

        p.SetSavedWindows(result);
    }
}


void MDIMainWindow::restoreMdiWindows()
{
    const Preferences &p = Preferences::Instance();
    if(p.GetSaveWindowLayout())
    {
        restoreGeometry(p.GetMainWindowGeometry());

        QList<MDIWindowInfo> windows = p.GetSavedWindows();
        foreach(MDIWindowInfo window, windows)
        {
            if(window.name=="Scope")
            {
                ScopeWindow *scopeWindow = new ScopeWindow(MIN_SACN_UNIVERSE, this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(scopeWindow);
                subWindow->restoreGeometry(window.geometry);
                scopeWindow->show();
            }

            if(window.name=="Universe")
            {
                UniverseView *universe = new UniverseView(MIN_SACN_UNIVERSE, this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(universe);
                subWindow->restoreGeometry(window.geometry);
                universe->show();
            }

            if(window.name=="Transmit")
            {
                transmitwindow *transmit = new transmitwindow(MIN_SACN_UNIVERSE, this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(transmit);
                subWindow->restoreGeometry(window.geometry);
                transmit->show();
            }

            if(window.name=="Snapshot")
            {
                Snapshot *snapshot = new Snapshot(MIN_SACN_UNIVERSE,this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(snapshot);
                subWindow->restoreGeometry(window.geometry);
                snapshot->show();
            }
            if(window.name=="MultiUniverse")
            {
                MultiUniverse *multi = new MultiUniverse(MIN_SACN_UNIVERSE,this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(multi);
                subWindow->restoreGeometry(window.geometry);
                multi->show();
            }
        }
    }
}

void MDIMainWindow::on_actionPCAPPlayback_triggered()
{
    PcapPlayback *pcapPlayback = new PcapPlayback(this);
    ui->mdiArea->addSubWindow(pcapPlayback);
    pcapPlayback->show();
}

int MDIMainWindow::getSelectedUniverse()
{
    QModelIndex selectedIndex = ui->treeView->currentIndex();
    int selectedUniverse = m_model->indexToUniverse(selectedIndex);
    return (selectedUniverse >= MIN_SACN_UNIVERSE && selectedUniverse<=MAX_SACN_UNIVERSE) ? selectedUniverse : 1;
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
