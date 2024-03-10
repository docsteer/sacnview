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

#include <Qt>
#include <QSettings>
#include <QPalette>
#include <QStyle>
#include <QApplication>
#include "preferences.h"
#include "consts.h"
#include <QRandomGenerator>
#include <cmath>

#if (defined(_WIN32) || defined(WIN32))
// On Windows, use the Qt zlib
#include <QtZlib/zlib.h>
#else
// Otherwise use the system zlib
#include <zlib.h>
#endif

// Strings for storing settings
static const QString S_INTERFACE_ADDRESS = QStringLiteral("MacAddress");
static const QString S_INTERFACE_NAME = QStringLiteral("InterfaceName");
static const QString S_DISPLAY_FORMAT = QStringLiteral("Display Format");
static const QString S_BLIND_VISUALIZER = QStringLiteral("Show Blind");
static const QString S_ETC_DDONLY = QStringLiteral("Show ETC DD Only");
static const QString S_ETC_DD = QStringLiteral("Enable ETC DD");
static const QString S_DEFAULT_SOURCENAME = QStringLiteral("Default Transmit Source Name");
static const QString S_TIMEOUT = QStringLiteral("Timeout");
static const QString S_FLICKERFINDERSHOWINFO = QStringLiteral("Flicker Finder Info");
static const QString S_SAVEWINDOWLAYOUT = QStringLiteral("Save Window Layout");
static const QString S_PRESETS = QStringLiteral("Preset %1");
static const QString S_MAINWINDOWGEOM = QStringLiteral("Main Window Geometry");
static const QString S_MAINWINDOWSTATE = QStringLiteral("Main Window State");
static const QString S_SUBWINDOWLIST = QStringLiteral("Sub Window");
static const QString S_SUBWINDOWNAME = QStringLiteral("SubWindow Name");
static const QString S_SUBWINDOWGEOM = QStringLiteral("SubWindow Geometry");
static const QString S_LISTEN_ALL = QStringLiteral("Listen All");
static const QString S_THEME = QStringLiteral("Theme");
static const QString S_TX_RATE_OVERRIDE = QStringLiteral("TX Rate Override");
static const QString S_LOCALE = QStringLiteral("LOCALE");
static const QString S_UNIVERSESLISTED = QStringLiteral("Universe List Count");
static const QString S_PRIORITYPRESET = QStringLiteral("PriorityPreset %1");
static const QString S_MULTICASTTTL = QStringLiteral("Multicast TTL");
static const QString S_PATHWAYSECURE_RX = QStringLiteral("Enable Pathway Secure Rx");
static const QString S_PATHWAYSECURE_RX_PASSWORD = QStringLiteral("Pathway Secure Rx Password");
static const QString S_PATHWAYSECURE_TX_PASSWORD = QStringLiteral("Pathway Secure Tx Password");
static const QString S_PATHWAYSECURE_RX_DATA_ONLY = QStringLiteral("Show Pathway Secure RX Data Only");
static const QString S_PATHWAYSECURE_RX_SEQUENCE_TIME_WINDOW = QStringLiteral("Pathway Secure Data RX Sequence Time Window");
static const QString S_PATHWAYSECURE_TX_SEQUENCE_TYPE = QStringLiteral("Pathway Secure Data TX Sequence Type");
static const QString S_PATHWAYSECURE_TX_SEQUENCE_BOOT_COUNT = QStringLiteral("Pathway Secure Data TX Sequence Boot Count");
static const QString S_PATHWAYSECURE_SEQUENCE_MAP = QStringLiteral("Pathway Secure Data Sequence Map");
static const QString S_UPDATE_IGNORE = QStringLiteral("Ignore Update Version");

// Floating window mode
static const QString S_WINDOW_MODE = QStringLiteral("WindowMode");  // MDI or floating mode
static const QString S_GROUP_FLOATING_WINDOW = QStringLiteral("FloatingWindows"); // Group for floating window geometries

// The base color to generate pastel shades for sources
static const QColor mixColor(QColorConstants::Svg::coral);

Preferences::Preferences()
{
  for (QByteArray& preset : m_presets)
    preset = QByteArray(MAX_DMX_ADDRESS, char(0));
  for (size_t i = 0; i < m_priorityPresets.size(); ++i)
    m_priorityPresets[i] = QByteArray(MAX_DMX_ADDRESS, char(100 + i));
  loadPreferences();
}

Preferences::~Preferences()
{
  savePreferences();
}

Preferences& Preferences::Instance()
{
  static Preferences s_instance;
  return s_instance;
}

QNetworkInterface Preferences::networkInterface() const
{
  if (!m_interface.isValid())
  {
    const auto allInts = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : allInts)
    {
      if (interface.flags().testFlag(QNetworkInterface::IsLoopBack))
        return interface;
    }
  }
  return m_interface;
}

void Preferences::setNetworkInterface(const QNetworkInterface& value)
{
  if (m_interface.name() != value.name())
  {
    m_interface = value;
    //emit networkInterfaceChanged();
  }
}

bool Preferences::interfaceSuitable(const QNetworkInterface& iface)
{
  if (iface.isValid() && (
    (
      // Up, can multicast...
      iface.flags().testFlag(QNetworkInterface::IsRunning)
      && iface.flags().testFlag(QNetworkInterface::IsUp)
      && iface.flags().testFlag(QNetworkInterface::CanMulticast)
      ) || (
        // Up, is loopback...
        iface.flags().testFlag(QNetworkInterface::IsRunning)
        && iface.flags().testFlag(QNetworkInterface::IsUp)
        && iface.flags().testFlag(QNetworkInterface::IsLoopBack)
        )
    )
    )
  {
    // ...has IPv4
    for (const QNetworkAddressEntry& addr : iface.addressEntries())
    {
      if (addr.ip().protocol() == QAbstractSocket::IPv4Protocol)
        return true;
    }
  }
  return false;
}

QString Preferences::GetIPv4AddressString(const QNetworkInterface& iface)
{
  // List IPv4 Addresses
  QString ipString;
  for (const QNetworkAddressEntry& e : iface.addressEntries())
  {
    if (e.ip().protocol() == QAbstractSocket::IPv4Protocol)
    {
      ipString.append(e.ip().toString());
      ipString.append(QLatin1Char(','));
    }
  }
  ipString.chop(1);
  return ipString;
}

QColor Preferences::colorForCID(const CID& cid) const
{
  const auto existing = m_cidToColor.find(cid);
  if (existing != m_cidToColor.end())
    return existing.value();

  // Use the zlib crc32 implementation to get a consistent checksum for a given CID
  quint8 cid_buf[CID::CIDBYTES] = {};
  cid.Pack(cid_buf);
  quint32 id = crc32(crc32(0L, Z_NULL, 0), (const Bytef*)cid_buf, CID::CIDBYTES);

  // Create a reasonable spread of different colors
  constexpr double golden_ratio = 0.618033988749895;
  double hue = golden_ratio * id;
  hue = std::fmod(hue, 1.0);
  double saturation = golden_ratio * id * 2;
  saturation = std::fmod(saturation, 0.25);
  saturation += 0.75; // High saturation

  // Choose lightness based on theme
  const double lightness = GetTheme() == Themes::DARK ? 0.25 : 0.5;
  QColor newColor = QColor::fromHslF(hue, saturation, lightness);
  m_cidToColor[cid] = newColor;
  return newColor;
}

QColor Preferences::colorForStatus(Status status) const
{
  switch (GetTheme())
  {
  case Themes::LIGHT: switch (status)
  {
  case Status::Good: return Qt::green;
  case Status::Warning: return Qt::yellow;
  case Status::Bad: return Qt::red;
  }
  case Themes::DARK: switch (status)
  {
  case Status::Good: return Qt::darkGreen;
  case Status::Warning: return Qt::darkYellow;
  case Status::Bad: return Qt::darkRed;
  }
  }
  return QColor();
}

void Preferences::SetDefaultTransmitName(const QString& sDefaultTransmitName)
{
  m_sDefaultTransmitName = sDefaultTransmitName.trimmed();
  m_sDefaultTransmitName.truncate(MAX_SOURCE_NAME_LEN);
  // Don't want to leave whitespace at end
  m_sDefaultTransmitName = m_sDefaultTransmitName.trimmed();
}

void Preferences::SetNumSecondsOfSacn(int nNumSecondsOfSacn)
{
  Q_ASSERT(nNumSecondsOfSacn >= 0 && nNumSecondsOfSacn <= MAX_SACN_TRANSMIT_TIME_SEC);
  m_nNumSecondsOfSacn = nNumSecondsOfSacn;
  return;
}

void Preferences::SetPreset(const QByteArray& data, int index)
{
  Q_ASSERT(index >= 0);
  Q_ASSERT(index < PRESET_COUNT);

  m_presets[index] = data;
}

const QByteArray& Preferences::GetPreset(int index) const
{
  Q_ASSERT(index >= 0);
  Q_ASSERT(index < PRESET_COUNT);

  return m_presets[index];
}

void Preferences::SetWindowMode(WindowMode mode)
{
  if (mode == m_windowMode)
    return;

  // Switch mode and load settings for new mode
  m_windowMode = mode;
  loadWindowGeometrySettings();
}

void Preferences::SetNetworkListenAll(const bool& value)
{
  if (networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack)) return;
  m_interfaceListenAll = value;
}

bool Preferences::GetNetworkListenAll() const
{
  if (networkInterface().flags().testFlag(QNetworkInterface::IsLoopBack)) return false;
  return m_interfaceListenAll;
}

void Preferences::SetPriorityPreset(const QByteArray& data, int index)
{
  Q_ASSERT(index < PRIORITYPRESET_COUNT);
  m_priorityPresets[index] = data;
}

const QByteArray& Preferences::GetPriorityPreset(int index) const
{
  Q_ASSERT(index < PRIORITYPRESET_COUNT);
  return m_priorityPresets[index];
}

void Preferences::SetPathwaySecureSequenceMap(const QByteArray& map)
{
  m_pathwaySecureSequenceMap = qCompress(map, 9);
}

QByteArray Preferences::GetPathwaySecureSequenceMap() const
{
  return qUncompress(m_pathwaySecureSequenceMap);
}

void Preferences::SetUpdateIgnore(const QString& version)
{
  QSettings settings;
  settings.setValue(S_UPDATE_IGNORE, version);
}

QString Preferences::GetUpdateIgnore() const
{
  QSettings settings;
  return settings.value(S_UPDATE_IGNORE, QString()).toString();
}

QString Preferences::GetFormattedValue(unsigned int nLevelInDecimal, bool decorated) const
{
  Q_ASSERT(nLevelInDecimal <= 255);

  switch (m_nDisplayFormat)
  {
  case DisplayFormat::DECIMAL: return QString::number(nLevelInDecimal, 10);
  case DisplayFormat::PERCENT:
  {
    QString result = QString::number(HTOPT[nLevelInDecimal], 10);
    if (decorated) return result + QLatin1Char('%');
    return result;
  }
  case DisplayFormat::HEXADECIMAL:
  {
    QString result = QString::number(nLevelInDecimal, 16);
    if (decorated) return QStringLiteral("0x") + result;
    return result;
  }
  default:
    return QStringLiteral("Err");
  }
}

void Preferences::savePreferences() const
{
  QSettings settings;

  if (m_interface.isValid())
  {
    settings.setValue(S_INTERFACE_ADDRESS, m_interface.hardwareAddress());
    settings.setValue(S_INTERFACE_NAME, m_interface.name());
  }
  settings.setValue(S_DISPLAY_FORMAT, QVariant(static_cast<int>(m_nDisplayFormat)));
  settings.setValue(S_BLIND_VISUALIZER, QVariant(m_bBlindVisualizer));
  settings.setValue(S_ETC_DDONLY, QVariant(m_bETCDisplayDDOnly));
  settings.setValue(S_ETC_DD, QVariant(m_bETCDD));
  settings.setValue(S_DEFAULT_SOURCENAME, m_sDefaultTransmitName);
  settings.setValue(S_TIMEOUT, QVariant(m_nNumSecondsOfSacn));
  settings.setValue(S_FLICKERFINDERSHOWINFO, QVariant(m_flickerFinderShowInfo));
  settings.setValue(S_SAVEWINDOWLAYOUT, m_saveWindowLayout);
  settings.setValue(S_WINDOW_MODE, static_cast<int>(m_windowMode));
  settings.setValue(S_LISTEN_ALL, m_interfaceListenAll);
  settings.setValue(S_THEME, m_theme);
  settings.setValue(S_TX_RATE_OVERRIDE, m_txrateoverride);
  settings.setValue(S_LOCALE, m_locale);
  settings.setValue(S_UNIVERSESLISTED, m_universesListed);

  saveWindowGeometrySettings();

  for (int i = 0; i < PRESET_COUNT; i++)
  {
    settings.setValue(S_PRESETS.arg(i), QVariant(m_presets[i]));
  }

  for (int i = 0; i < PRIORITYPRESET_COUNT; i++)
  {
    settings.setValue(S_PRIORITYPRESET.arg(i), QVariant(m_priorityPresets[i]));
  }

  settings.setValue(S_MULTICASTTTL, m_multicastTtl);

  settings.setValue(S_PATHWAYSECURE_RX, m_pathwaySecureRx);
  settings.setValue(S_PATHWAYSECURE_RX_PASSWORD, m_pathwaySecureRxPassword);
  settings.setValue(S_PATHWAYSECURE_TX_PASSWORD, m_pathwaySecureTxPassword);
  settings.setValue(S_PATHWAYSECURE_RX_DATA_ONLY, m_pathwaySecureRxDataOnly);
  settings.setValue(S_PATHWAYSECURE_TX_SEQUENCE_TYPE, m_pathwaySecureTxSequenceType);
  settings.setValue(S_PATHWAYSECURE_TX_SEQUENCE_BOOT_COUNT, m_pathwaySecureTxSequenceBootCount);
  settings.setValue(S_PATHWAYSECURE_RX_SEQUENCE_TIME_WINDOW, m_pathwaySecureRxSequenceTimeWindow);
  settings.setValue(S_PATHWAYSECURE_SEQUENCE_MAP, m_pathwaySecureSequenceMap);

  settings.sync();
}

void Preferences::loadPreferences()
{
  QSettings settings;

  if (settings.contains(S_INTERFACE_ADDRESS))
  {
    const QString mac = settings.value(S_INTERFACE_ADDRESS).toString();
    const auto interFromName = QNetworkInterface::interfaceFromName(settings.value(S_INTERFACE_NAME).toString());
    if (interFromName.isValid() && (interFromName.hardwareAddress() == mac))
    {
      m_interface = interFromName;
    }
    else {
      QList<QNetworkInterface> ifaceList = QNetworkInterface::allInterfaces();
      for (const auto& i : ifaceList)
        if (i.hardwareAddress() == mac)
          m_interface = i;
    }
  }

  m_interfaceListenAll = settings.value(S_LISTEN_ALL, m_interfaceListenAll).toBool();
  m_nDisplayFormat = static_cast<DisplayFormat>(settings.value(S_DISPLAY_FORMAT, static_cast<int>(m_nDisplayFormat)).toInt());
  m_bBlindVisualizer = settings.value(S_BLIND_VISUALIZER, m_bBlindVisualizer).toBool();
  m_bETCDisplayDDOnly = settings.value(S_ETC_DDONLY, m_bETCDisplayDDOnly).toBool();
  m_bETCDD = settings.value(S_ETC_DD, m_bETCDD).toBool();
  m_sDefaultTransmitName = settings.value(S_DEFAULT_SOURCENAME, m_sDefaultTransmitName).toString();
  m_nNumSecondsOfSacn = settings.value(S_TIMEOUT, m_nNumSecondsOfSacn).toInt();
  m_flickerFinderShowInfo = settings.value(S_FLICKERFINDERSHOWINFO, m_flickerFinderShowInfo).toBool();
  m_windowMode = static_cast<WindowMode>(settings.value(S_WINDOW_MODE, static_cast<int>(m_windowMode)).toInt());
  m_saveWindowLayout = settings.value(S_SAVEWINDOWLAYOUT, m_saveWindowLayout).toBool();
  m_theme = static_cast<Themes::theme_e>(settings.value(S_THEME, static_cast<int>(m_theme)).toInt());
  m_txrateoverride = settings.value(S_TX_RATE_OVERRIDE, m_txrateoverride).toBool();
  m_locale = settings.value(S_LOCALE, m_locale).toLocale();
  m_universesListed = settings.value(S_UNIVERSESLISTED, m_universesListed).toUInt();
  m_multicastTtl = settings.value(S_MULTICASTTTL, m_multicastTtl).toUInt();
  m_pathwaySecureRx = settings.value(S_PATHWAYSECURE_RX, m_pathwaySecureRx).toBool();
  m_pathwaySecureRxPassword = settings.value(S_PATHWAYSECURE_RX_PASSWORD, m_pathwaySecureRxPassword).toString();
  m_pathwaySecureTxPassword = settings.value(S_PATHWAYSECURE_TX_PASSWORD, m_pathwaySecureTxPassword).toString();
  m_pathwaySecureRxDataOnly = settings.value(S_PATHWAYSECURE_RX_DATA_ONLY, m_pathwaySecureRxDataOnly).toBool();
  m_pathwaySecureTxSequenceType = settings.value(S_PATHWAYSECURE_TX_SEQUENCE_TYPE, m_pathwaySecureTxSequenceType).toUInt();
  m_pathwaySecureTxSequenceBootCount = settings.value(S_PATHWAYSECURE_TX_SEQUENCE_BOOT_COUNT, m_pathwaySecureTxSequenceBootCount).toUInt();
  m_pathwaySecureRxSequenceTimeWindow = settings.value(S_PATHWAYSECURE_RX_SEQUENCE_TIME_WINDOW, m_pathwaySecureRxSequenceTimeWindow).toUInt();
  m_pathwaySecureSequenceMap = settings.value(S_PATHWAYSECURE_SEQUENCE_MAP, m_pathwaySecureSequenceMap).toByteArray();

  loadWindowGeometrySettings();

  for (int i = 0; i < PRESET_COUNT; i++) {
    if (settings.contains(S_PRESETS.arg(i))) {
      // Never change the size
      m_presets[i].replace(0, MAX_DMX_ADDRESS, settings.value(S_PRESETS.arg(i)).toByteArray());
    }
  }

  for (int i = 0; i < PRIORITYPRESET_COUNT; i++) {
    if (settings.contains(S_PRIORITYPRESET.arg(i))) {
      // Never change the size
      m_priorityPresets[i].replace(0, MAX_DMX_ADDRESS, settings.value(S_PRIORITYPRESET.arg(i)).toByteArray());
    }
  }
}

void Preferences::loadWindowGeometrySettings()
{
  QSettings settings;
  switch (m_windowMode)
  {
  default: break;
  case WindowMode::Floating:
    settings.beginGroup(S_GROUP_FLOATING_WINDOW);
    break;
  }

  m_mainWindowGeometry.first = settings.value(S_MAINWINDOWGEOM, m_mainWindowGeometry.first).toByteArray();
  m_mainWindowGeometry.second = settings.value(S_MAINWINDOWSTATE, m_mainWindowGeometry.second).toByteArray();

  m_windowInfo.clear();
  int size = settings.beginReadArray(S_SUBWINDOWLIST);
  for (int i = 0; i < size; i++)
  {
    SubWindowInfo value;
    settings.setArrayIndex(i);
    value.name = settings.value(S_SUBWINDOWNAME).toString();
    value.geometry = settings.value(S_SUBWINDOWGEOM).toByteArray();
    m_windowInfo << value;
  }
  settings.endArray();
}

void Preferences::saveWindowGeometrySettings() const
{
  QSettings settings;
  switch (m_windowMode)
  {
  default: break;
  case WindowMode::Floating:
    settings.beginGroup(S_GROUP_FLOATING_WINDOW);
    break;
  }

  settings.setValue(S_MAINWINDOWGEOM, m_mainWindowGeometry.first);
  settings.setValue(S_MAINWINDOWSTATE, m_mainWindowGeometry.second);

  settings.beginWriteArray(S_SUBWINDOWLIST);
  for (int i = 0; i < m_windowInfo.count(); i++)
  {
    settings.setArrayIndex(i);
    settings.setValue(S_SUBWINDOWNAME, m_windowInfo[i].name);
    settings.setValue(S_SUBWINDOWGEOM, m_windowInfo[i].geometry);
  }
  settings.endArray();
}
