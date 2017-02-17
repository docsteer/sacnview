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
#include "preferencesdialog.h"
#include "aboutdialog.h"
#include "sacnuniverselistmodel.h"
#include "snapshot.h"

MDIMainWindow::MDIMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MDIMainWindow), m_model(NULL)
{
    ui->setupUi(this);
    ui->sbUniverseList->setMinimum(MIN_SACN_UNIVERSE);
    ui->sbUniverseList->setMaximum(MAX_SACN_UNIVERSE - NUM_UNIVERSES_LISTED);


    m_model = new sACNUniverseListModel(this);
    ui->treeView->setModel(m_model);
    ui->treeView->expandAll();
    connect(ui->treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(universeDoubleClick(QModelIndex)));
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
