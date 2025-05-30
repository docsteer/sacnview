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

#include "preferencesdialog.h"
#include "src/sacn/sacnlistener.h"
#include "ui_preferencesdialog.h"
#include "preferences.h"
#include "consts.h"
#include <sstream>
#include <QMessageBox>
#include <QListWidget>
#include "streamingacn.h"
#include "securesacn.h"

PreferencesDialog::PreferencesDialog(QWidget* parent) :
  QDialog(parent),
  ui(new Ui::PreferencesDialog)
{
  ui->setupUi(this);

  // Populate lang
  m_translation = new TranslationDialog(Preferences::Instance().GetLocale(), ui->vlLanguage, this);
  // Add language spacer
  ui->vlLanguage->addStretch();

  // Pathway TX sequence type
  ui->cmbPathwayTxSequenceType->addItem(tr("Time"));
  ui->cmbPathwayTxSequenceType->addItem(tr("Volatile"));
  ui->cmbPathwayTxSequenceType->addItem(tr("Non-Volatile"));
  ui->cmbPathwayTxSequenceType->setCurrentIndex(0); // Default Time

  connect(ui->btnSaveWindows, &QAbstractButton::clicked, this, &PreferencesDialog::storeWindowLayoutNow);

  // Category selection
  connect(ui->lwPrefCategory, &QListWidget::itemSelectionChanged, [this]() {
      const auto item = ui->lwPrefCategory->currentItem();
      const auto index = ui->lwPrefCategory->indexFromItem(item);
      ui->stackedWidget->setCurrentIndex(index.row());
  });

  ui->lwPrefCategory->setCurrentRow(0);
}

PreferencesDialog::~PreferencesDialog()
{
  delete ui;
}


void PreferencesDialog::showEvent(QShowEvent* e)
{
  // Network interfaces
  m_interfaceList.clear();
  ui->cmbNetInterface->clear();

  QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
  for (const QNetworkInterface& iface : interfaces)
  {
    // If the interface is ok for use...
    if (Preferences::Instance().interfaceSuitable(iface))
    {
      // List IPv4 Addresses
      const QString ipString = Preferences::GetIPv4AddressString(iface);

      ui->cmbNetInterface->addItem(QStringLiteral("%1 (%2)")
        .arg(iface.humanReadableName())
        .arg(ipString));

      // Assign by MAC or Name
      if (Preferences::Instance().networkInterface().hardwareAddress() == iface.hardwareAddress() ||
        Preferences::Instance().networkInterface().name() == iface.name())
        ui->cmbNetInterface->setCurrentIndex(ui->cmbNetInterface->count() - 1);

      m_interfaceList << iface;
    }
  }
  ui->cbListenAll->setChecked(Preferences::Instance().GetNetworkListenAll());
  if (Preferences::Instance().networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack))
    ui->cbListenAll->setEnabled(false);

  switch (Preferences::Instance().GetDisplayFormat())
  {
  case DisplayFormat::DECIMAL:       ui->DecimalDisplayFormat->setChecked(true); break;
  case DisplayFormat::PERCENT:       ui->PercentDisplayFormat->setChecked(true); break;
  case DisplayFormat::HEXADECIMAL:   ui->HexDisplayFormat->setChecked(true); break;
  case DisplayFormat::COUNT:         break;
  }

  ui->cbDisplayBlind->setChecked(Preferences::Instance().GetBlindVisualizer());
  ui->cbETCDisplayDDOnlys->setChecked(Preferences::Instance().GetETCDisplayDDOnly());
  ui->gbETCDD->setChecked(Preferences::Instance().GetETCDD());

  ui->gbPathwaySecureRx->setChecked(Preferences::Instance().GetPathwaySecureRx());
  ui->lePathwaySecureRxPassword->setText(Preferences::Instance().GetPathwaySecureRxPassword());
  ui->lePathwaySecureTxPassword->setText(Preferences::Instance().GetPathwaySecureTxPassword());
  ui->cbPathwaySecureRxDataOnly->setChecked(Preferences::Instance().GetPathwaySecureRxDataOnly());
  ui->sbPathwaySecureRxSequenceTimeWindow->setValue(Preferences::Instance().GetPathwaySecureRxSequenceTimeWindow());
  ui->cmbPathwayTxSequenceType->setCurrentIndex(Preferences::Instance().GetPathwaySecureTxSequenceType());

  ui->cbRestoreWindows->setChecked(Preferences::Instance().GetRestoreWindowLayout());
  ui->cbAutoRx->setChecked(Preferences::Instance().GetAutoStartRX());
  // Can't automatically start receiving unless restoring the windows
  ui->cbAutoRx->setEnabled(Preferences::Instance().GetRestoreWindowLayout());
  connect(ui->cbRestoreWindows, &QCheckBox::toggled, ui->cbAutoRx, &QCheckBox::setEnabled);

  ui->cbSaveWindowsOnExit->setChecked(Preferences::Instance().GetAutoSaveWindowLayout());
  // Can't manually save layout if autosaving as will get wiped
  ui->btnSaveWindows->setDisabled(Preferences::Instance().GetAutoSaveWindowLayout());
  connect(ui->cbSaveWindowsOnExit, &QCheckBox::toggled, ui->btnSaveWindows, &QPushButton::setDisabled);

  ui->cbFloatingWindows->setChecked(Preferences::Instance().GetWindowMode() == WindowMode::Floating);

  ui->leDefaultSourceName->setText(Preferences::Instance().GetDefaultTransmitName());

  int timeout = Preferences::Instance().GetNumSecondsOfSacn();
  if (timeout > 0)
  {
    ui->gbTransmitTimeout->setChecked(true);
    int hour = (timeout / (60 * 60));
    int min = ((timeout / 60) - (hour * 60));
    int sec = (timeout - (hour * 60 * 60) - (min * 60));
    ui->NumOfHoursOfSacn->setValue(hour);
    ui->NumOfMinOfSacn->setValue(min);
    ui->NumOfSecOfSacn->setValue(sec);
  }
  else
  {
    ui->gbTransmitTimeout->setChecked(false);
  }

  ui->cbTxRateOverride->setChecked(Preferences::Instance().GetTXRateOverride());
  ui->cbTxBadPriority->setChecked(Preferences::Instance().GetTXBadPriority());

  ui->cmbTheme->clear();
  ui->cmbTheme->addItems(Themes::getDescriptions());
  ui->cmbTheme->setCurrentIndex(static_cast<int>(Preferences::Instance().GetTheme()));

  ui->sbMulticastTtl->setValue(Preferences::Instance().GetMulticastTtl());
}

void PreferencesDialog::on_buttonBox_accepted()
{
  bool requiresRestart = false;

  Preferences& p = Preferences::Instance();

  // Display Format
  if (ui->DecimalDisplayFormat->isChecked())
    p.SetDisplayFormat(DisplayFormat::DECIMAL);
  else if (ui->HexDisplayFormat->isChecked())
    p.SetDisplayFormat(DisplayFormat::HEXADECIMAL);
  else if (ui->PercentDisplayFormat->isChecked())
    p.SetDisplayFormat(DisplayFormat::PERCENT);

  // Display Blind
  p.SetBlindVisualizer(ui->cbDisplayBlind->isChecked());

  p.SetMergeIllegalPriorities(ui->cbRxIllegalPriorities->isChecked());

  // Enable ETC DD?
  p.SetETCDD(ui->gbETCDD->isChecked());

  // Display sources with only ETC DD?
  if (ui->cbETCDisplayDDOnlys->isChecked() != p.GetETCDisplayDDOnly()) { requiresRestart = true; }
  p.SetETCDisplayDDOnly(ui->cbETCDisplayDDOnlys->isChecked());

  // Pathway Secure
  p.SetPathwaySecureRx(ui->gbPathwaySecureRx->isChecked());
  p.SetPathwaySecureRxPassword(ui->lePathwaySecureRxPassword->text());
  p.SetPathwaySecureTxPassword(ui->lePathwaySecureTxPassword->text());
  p.SetPathwaySecureRxDataOnly(ui->cbPathwaySecureRxDataOnly->isChecked());
  p.SetPathwaySecureRxSequenceTimeWindow(ui->sbPathwaySecureRxSequenceTimeWindow->value());

  switch (ui->cmbPathwayTxSequenceType->currentIndex())
  {
  default:
  case PathwaySecure::Sequence::type_time:
    p.SetPathwaySecureTxSequenceType(PathwaySecure::Sequence::type_time);
    break;

  case PathwaySecure::Sequence::type_volatile:
    p.SetPathwaySecureTxSequenceType(PathwaySecure::Sequence::type_volatile);
    break;

  case PathwaySecure::Sequence::type_nonvolatile:
    p.SetPathwaySecureTxSequenceType(PathwaySecure::Sequence::type_nonvolatile);
    break;
  }

  // Windowing mode
  if (ui->cbFloatingWindows->isChecked())
    p.SetWindowMode(WindowMode::Floating);
  else
    p.SetWindowMode(WindowMode::MDI);

  // Save layout
  p.SetRestoreWindowLayout(ui->cbRestoreWindows->isChecked());
  p.SetAutoSaveWindowLayout(ui->cbSaveWindowsOnExit->isChecked());

  // Receive autostart
  Preferences::Instance().SetAutoStartRX(ui->cbAutoRx->isChecked());

  // Transmit timeout
  int seconds = ui->NumOfHoursOfSacn->value() * 60 * 60 + ui->NumOfMinOfSacn->value() * 60 + ui->NumOfSecOfSacn->value();
  if (!ui->gbTransmitTimeout->isChecked())
    seconds = 0;
  p.SetNumSecondsOfSacn(seconds);

  // Default source name
  p.SetDefaultTransmitName(ui->leDefaultSourceName->text());

  if (ui->cbTxRateOverride->isChecked() != p.GetTXRateOverride())
    requiresRestart = true;
  p.SetTXRateOverride(ui->cbTxRateOverride->isChecked());

  if (ui->cbTxBadPriority->isChecked() != p.GetTXBadPriority())
    requiresRestart = true;
  p.SetTXBadPriority(ui->cbTxBadPriority->isChecked());

  // Interfaces
  {
    const int i = ui->cmbNetInterface->currentIndex();
    if (i >= 0 && i < m_interfaceList.size())
    {
      QNetworkInterface interface = m_interfaceList[i];
      if (interface.index() != p.networkInterface().index())
      {
        p.setNetworkInterface(m_interfaceList[i]);

        requiresRestart = true;
      }
    }
  }

  requiresRestart |= ui->cbListenAll->isChecked() != p.GetNetworkListenAll();
  p.SetNetworkListenAll(ui->cbListenAll->isChecked());

  // Theme
  auto theme = static_cast<Themes::theme_e>(ui->cmbTheme->currentIndex());
  if (p.GetTheme() != theme)
  {
    p.SetTheme(theme);
    requiresRestart = true;
  }

  // Language
  if (m_translation->GetSelectedLocale() != p.GetLocale())
  {
    p.SetLocale(m_translation->GetSelectedLocale());
    requiresRestart = true;
  }

  // Multicast TTL
  if (p.GetMulticastTtl() != ui->sbMulticastTtl->value())
  {
    p.SetMulticastTtl(ui->sbMulticastTtl->value());
    requiresRestart = true;
  }

  // Restart to apply?
  if (requiresRestart) {
    QMessageBox::information(this, tr("Restart required"),
      tr("To apply these preferences, you will need to restart the application. \nsACNView will now close and restart"),
      QMessageBox::Ok);
    p.SetRestartPending();
    qApp->quit();
    return;
  }

  // Force all universes to perform a full remerge
  const auto& listenerList = sACNManager::Instance().getListenerList();
  for (const auto& weakListener : listenerList) {

    sACNManager::tListener listener(weakListener);
    if (listener)
      listener->doFullMerge();
  }

  accept();
}
