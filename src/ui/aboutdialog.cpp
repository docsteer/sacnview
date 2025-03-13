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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "consts.h"
#include "pcap/pcapplayback.h"
#include "preferences.h"
#include "translations/translations.h"
#include "newversiondialog.h"
#include "sacnlistenermodel.h"

#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QLibraryInfo>

aboutDialog::aboutDialog(QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::aboutDialog)
{
  m_versionCheck = new VersionCheck(this);

  ui->setupUi(this);

  if (QString(VERSION) == QString(GIT_CURRENT_SHA1)) {
    ui->DisplayVer->setText(QString("%1").arg(VERSION));
  }
  else {
    ui->DisplayVer->setText(QString("%1\n%2")
      .arg(VERSION, GIT_CURRENT_SHA1));
  }
  ui->displayDate->setText(QString("%1, %2 %3 %4")
    .arg(GIT_DATE_DAY, GIT_DATE_DATE, GIT_DATE_MONTH, GIT_DATE_YEAR));
  ui->DisplayName->setText(AUTHORS.join("\n"));

  // Translators
  QStringList translators;
  for (const auto& translations : Translations::lTranslations)
  {
    for (const auto& translatorName : translations.Translators)
    {
      if (!translators.contains(translatorName))
        translators << translatorName;
    }
  }
  ui->lblTranslatorNames->setText(translators.join("\n"));

  // Automatic updates
  ui->chkAutoUpdate->setChecked(Preferences::Instance().GetAutoCheckUpdates());

  // Libs
  ui->lblLicense->setText(
    tr("<p>This application is provided under the <a href=\"http://www.apache.org/licenses/LICENSE-2.0\">Apache License, version 2.0</a></p>")
  );

  {
    QString licence_string = tr("Licensed under the <a href=\"http://www.gnu.org/licenses/lgpl.html\">GNU LGPL</a>");
    const QVersionNumber qt_ver = QLibraryInfo::version();
    // Qt 5.15.3 was the last LGPL Qt5 
    if (qt_ver.majorVersion() == 5 && qt_ver.minorVersion() == 15 && qt_ver.microVersion() > 3)
      licence_string = tr("Licenced commercially");

    ui->lblQtInfo->setText(
      tr("<p>This application uses the Qt Library<br>"
        "Version %1<br>%2</p>")
      .arg(qt_ver.toString(), licence_string));
  }

  {
    ui->lblLibs->clear();
    if (PcapPlayback::foundLibPcap())
    {
        ui->lblLibs->setText(
          tr("<p>This application uses <a href=\"https://www.tcpdump.org/\">libpcap</a><br>"
            "%1<br>"
            "Licensed under the <a href=\"https://opensource.org/licenses/BSD-3-Clause\">The 3-Clause BSD License</a></p>")
              .arg(QString::fromStdString(PcapPlayback::getLibPcapVersion())));
    }
    ui->lblLibs->setText(ui->lblLibs->text() +
      tr("<p>This application uses <a href=\"https://www.blake2.net/\">BLAKE2</a><br>"
        "Licensed under the <a href=\"https://creativecommons.org/publicdomain/zero/1.0/\">Creative Commons Zero v1.0 Universal</a></p>"));
  }

  // Setup diagnostics
  m_listenerModel = new SACNListenerTableModel(this);
  QSortFilterProxyModel* sortProxy = new QSortFilterProxyModel(this);
  sortProxy->setSourceModel(m_listenerModel);
  sortProxy->setDynamicSortFilter(true);
  ui->twDiag->setModel(sortProxy);
  ui->twDiag->sortByColumn(SACNListenerTableModel::COL_UNIVERSE, Qt::AscendingOrder);

  resizeDiagColumn();

  m_displayTimer = new QTimer(this);
  connect(m_displayTimer, &QTimer::timeout, this, &aboutDialog::updateDisplay);
  m_displayTimer->start(1000);
}

aboutDialog::~aboutDialog()
{
  delete m_listenerModel;
  m_displayTimer->stop();
  delete ui;
}

void aboutDialog::updateDisplay()
{
  if (m_listenerModel)
    m_listenerModel->refresh();
}

void aboutDialog::resizeDiagColumn()
{
  ui->twDiag->resizeColumnsToContents();
}

void aboutDialog::on_chkAutoUpdate_clicked(bool checked)
{
  Preferences::Instance().SetAutoCheckUpdates(checked);
}

void aboutDialog::on_btnCheckUpdateNow_clicked(bool)
{
  m_versionCheck->checkForUpdate();
}

void aboutDialog::on_twDiag_expanded(const QModelIndex& index)
{
  Q_UNUSED(index);

  resizeDiagColumn();
}
void aboutDialog::on_twDiag_collapsed(const QModelIndex& index)
{
  Q_UNUSED(index);

  resizeDiagColumn();
}


void aboutDialog::on_aboutDialog_finished(int result)
{
  Q_UNUSED(result);

  this->deleteLater();
}
