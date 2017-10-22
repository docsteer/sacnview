// Copyright 2016 Tom Barthel-Steer
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
#include "snapshot.h"
#include "multiuniverse.h"
#include "pcapplayback.h"

#include <QMdiSubWindow>

MDIMainWindow::MDIMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MDIMainWindow), m_model(NULL)
{
    ui->setupUi(this);
    ui->sbUniverseList->setMinimum(MIN_SACN_UNIVERSE);
    ui->sbUniverseList->setMaximum(MAX_SACN_UNIVERSE - NUM_UNIVERSES_LISTED + 1);
    ui->sbUniverseList->setWrapping(true);
    ui->sbUniverseList->setValue(MIN_SACN_UNIVERSE);

    m_model = new sACNUniverseListModel(this);
    ui->treeView->setModel(m_model);
    ui->treeView->expandAll();
    connect(ui->treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(universeDoubleClick(QModelIndex)));
    connect(m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
}

MDIMainWindow::~MDIMainWindow()
{
    delete ui;
}

void MDIMainWindow::on_actionScopeView_triggered(bool checked)
{
    Q_UNUSED(checked);
    ScopeWindow *scopeWindow = new ScopeWindow(this);
    ui->mdiArea->addSubWindow(scopeWindow);
    scopeWindow->show();
}

void MDIMainWindow::on_actionRecieve_triggered(bool checked)
{
    Q_UNUSED(checked);
    UniverseView *uniView = new UniverseView(this);
    ui->mdiArea->addSubWindow(uniView);
    uniView->show();
}

void MDIMainWindow::on_actionTranmsit_triggered(bool checked)
{
    Q_UNUSED(checked);
    transmitwindow *trView = new transmitwindow();
    ui->mdiArea->addSubWindow(trView);
    trView->show();
}

void MDIMainWindow::on_actionSnapshot_triggered(bool checked)
{
    Q_UNUSED(checked);
    Snapshot *snapView = new Snapshot();
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
    ui->sbUniverseList->setValue(ui->sbUniverseList->value() - NUM_UNIVERSES_LISTED);
}

void MDIMainWindow::on_btnUnivListForward_pressed()
{

    ui->sbUniverseList->setValue(ui->sbUniverseList->value() + NUM_UNIVERSES_LISTED);
}

void MDIMainWindow::on_sbUniverseList_valueChanged(int value)
{
    if(m_model)
        m_model->setStartUniverse(value);
}


void MDIMainWindow::universeDoubleClick(const QModelIndex &index)
{
    if(!m_model) return;

    int universe = m_model->indexToUniverse(index);

    if(universe>0)
    {
        UniverseView *uniView = new UniverseView(this);
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
    MultiUniverse *multiUniv = new MultiUniverse(this);
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
    Preferences *p = Preferences::getInstance();
    if(p->GetSaveWindowLayout())
    {
        p->SetMainWindowGeometry(saveGeometry());

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
        }

        p->SetSavedWindows(result);
    }
}


void MDIMainWindow::restoreMdiWindows()
{
    Preferences *p = Preferences::getInstance();
    if(p->GetSaveWindowLayout())
    {
        restoreGeometry(p->GetMainWindowGeometry());

        QList<MDIWindowInfo> windows = p->GetSavedWindows();
        foreach(MDIWindowInfo window, windows)
        {
            if(window.name=="Scope")
            {
                ScopeWindow *scopeWindow = new ScopeWindow(this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(scopeWindow);
                subWindow->restoreGeometry(window.geometry);
                scopeWindow->show();
            }

            if(window.name=="Universe")
            {
                UniverseView *universe = new UniverseView(this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(universe);
                subWindow->restoreGeometry(window.geometry);
                universe->show();
            }

            if(window.name=="Transmit")
            {
                transmitwindow *transmit = new transmitwindow(this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(transmit);
                subWindow->restoreGeometry(window.geometry);
                transmit->show();
            }

            if(window.name=="Snapshot")
            {
                Snapshot *snapshot = new Snapshot(this);
                QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(snapshot);
                subWindow->restoreGeometry(window.geometry);
                snapshot->show();
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
