// Copyright (c) 2015 Tom Barthel-Steer, http://www.tomsteer.net
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "mdimainwindow.h"
#include "ui_mdimainwindow.h"
#include "scopewindow.h"
#include "universeview.h"
#include "transmitwindow.h"

MDIMainWindow::MDIMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MDIMainWindow)
{
    ui->setupUi(this);
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
