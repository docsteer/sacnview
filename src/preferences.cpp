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

///////////////////////////////
// Strings for storing settings

// Translation
static const QString S_LOCALE = QStringLiteral("Locale");

// Updates
static const QString S_UPDATE_IGNORE = QStringLiteral("IgnoreUpdateVersion");
static const QString S_UPDATE_IGNORE_ALL = QStringLiteral("All"); // Special value

// Local networkinng
static const QString S_INTERFACE_ADDRESS = QStringLiteral("MacAddress");
static const QString S_INTERFACE_NAME = QStringLiteral("InterfaceName");
static const QString S_LISTEN_ALL = QStringLiteral("ListenAllNics");

// Display/Theming
static const QString S_DISPLAY_FORMAT = QStringLiteral("DisplayFormat");
static const QString S_THEME = QStringLiteral("Theme");
static const QString S_WINDOW_MODE = QStringLiteral("WindowMode");  // MDI or floating mode

// Save/Restore windows
static const QString S_SAVEWINDOWLAYOUT = QStringLiteral("AutosaveWindowLayout");
static const QString S_RESTOREWINDOWLAYOUT = QStringLiteral("RestoreWindowLayout");

static const QString S_GROUP_FLOATING_WINDOW = QStringLiteral("Floating");  // Group for floating window geometries
static const QString S_GROUP_MDI_WINDOW = QStringLiteral("MDI");            // Group for MDI window geometries

static const QString S_SUBWINDOWLIST = QStringLiteral("SubWindow");

static const QString S_WINDOWNAME = QStringLiteral("Name");
static const QString S_WINDOWGEOM = QStringLiteral("Geometry");
static const QString S_WINDOWSTATE = QStringLiteral("State");
static const QString S_WINDOWCONFIG = QStringLiteral("Configuration");

// Receive
static const QString S_RX_AUTOSTART = QStringLiteral("Rx/AutomaticStart");
static const QString S_RX_UNIVERSESLIST_START = QStringLiteral("Rx/UniverseList/Start");
static const QString S_RX_UNIVERSESLIST_COUNT = QStringLiteral("Rx/UniverseList/Count");

static const QString S_RX_BLIND_VISUALIZER = QStringLiteral("Rx/ShowBlind");
static const QString S_RX_FLICKERFINDERSHOWINFO = QStringLiteral("Rx/FlickerFinderInfo");
static const QString S_RX_ETC_DD = QStringLiteral("Rx/EnableDD");
static const QString S_RX_ETC_DDONLY = QStringLiteral("Rx/ShowDDNoLevelSource");
static const QString S_RX_BAD_PRIORITY = QStringLiteral("Rx/MergeBadPriority");

// Pathway Receive
static const QString S_PATHWAYSECURE_RX = QStringLiteral("PathwaySecure/Rx/Enable");
static const QString S_PATHWAYSECURE_RX_PASSWORD = QStringLiteral("PathwaySecure/Rx/Password");
static const QString S_PATHWAYSECURE_RX_DATA_ONLY = QStringLiteral("PathwaySecure/Rx/SecureOnly");
static const QString S_PATHWAYSECURE_RX_SEQUENCE_TIME_WINDOW = QStringLiteral("PathwaySecure/Rx/SequenceTimeWindow");

// Transmit
static const QString S_TX_DEFAULT_SOURCENAME = QStringLiteral("Tx/DefaultSourceName");
static const QString S_TX_TIMEOUT = QStringLiteral("Tx/Timeout");

static const QString S_TX_RATE_OVERRIDE = QStringLiteral("Tx/RateOverride");
static const QString S_TX_BAD_PRIORITY = QStringLiteral("Tx/BadPriority");
static const QString S_TX_PRESETS = QStringLiteral("Tx/Preset/%1");
static const QString S_TX_PRIORITYPRESET = QStringLiteral("Tx/PriorityPreset/%1");
static const QString S_TX_MULTICAST_TTL = QStringLiteral("Tx/MulticastTTL");

// Pathway Transmit
static const QString S_PATHWAYSECURE_TX_PASSWORD = QStringLiteral("PathwaySecure/Tx/Password");
static const QString S_PATHWAYSECURE_TX_SEQUENCE_TYPE = QStringLiteral("PathwaySecure/Tx/SequenceType");
static const QString S_PATHWAYSECURE_TX_SEQUENCE_BOOT_COUNT = QStringLiteral("PathwaySecure/Tx/SequenceBootCount");

static const QString S_PATHWAYSECURE_SEQUENCE_MAP = QStringLiteral("PathwaySecure/SequenceMap");

// The base color to generate pastel shades for sources
static const QColor mixColor(QColorConstants::Svg::coral);

static const QByteArray DefaultByteArrayDmx(MAX_DMX_ADDRESS, char(0));
static QByteArray DefaultByteArrayPriority(size_t i)
{
  return QByteArray(MAX_DMX_ADDRESS, char(100 + i));
}

// Helper functions for simpler format
static void SaveByteArray(QSettings& settings, const QString& key, const QByteArray& value)
{
  if (value.isEmpty())
    settings.remove(key);
  else
    settings.setValue(key, value);
}

Preferences::Preferences()
{
  for (QByteArray& preset : m_presets)
    preset = DefaultByteArrayDmx;
  for (size_t i = 0; i < m_priorityPresets.size(); ++i)
    m_priorityPresets[i] = DefaultByteArrayPriority(i);

  // Allow the commandline to override the configuration file
  const QStringList args = QCoreApplication::arguments();
  // Find the last ini override
  auto pref_index = args.lastIndexOf(QStringLiteral("-ini"));
  if (pref_index == -1)
    pref_index = args.lastIndexOf(QStringLiteral("/ini"));

  if (pref_index != -1 && pref_index < args.size() - 1)
    m_settings_file = args[pref_index + 1];

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
  const auto id = crc32(crc32(0L, Z_NULL, 0), (const Bytef*)cid_buf, CID::CIDBYTES);

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
  QSettings settings = getSettings();
  settings.setValue(S_UPDATE_IGNORE, version);
}

QString Preferences::GetUpdateIgnore() const
{
  QSettings settings = getSettings();
  return settings.value(S_UPDATE_IGNORE, QString()).toString();
}

bool Preferences::GetAutoCheckUpdates() const
{
  return GetUpdateIgnore() != S_UPDATE_IGNORE_ALL;
}

void Preferences::SetAutoCheckUpdates(bool b)
{
  if (b)
    SetUpdateIgnore(QString());
  else
    SetUpdateIgnore(S_UPDATE_IGNORE_ALL);
}

QString Preferences::GetFormattedValue(int nLevelInDecimal, bool decorated) const
{
  // Negative means "none"
  if (nLevelInDecimal < 0)
    return QStringLiteral("---");

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
  QSettings settings = getSettings();

  if (m_interface.isValid())
  {
    settings.setValue(S_INTERFACE_ADDRESS, m_interface.hardwareAddress());
    settings.setValue(S_INTERFACE_NAME, m_interface.name());
  }
  settings.setValue(S_LISTEN_ALL, m_interfaceListenAll);

  settings.setValue(S_LOCALE, m_locale.bcp47Name());
  settings.setValue(S_THEME, m_theme);
  settings.setValue(S_DISPLAY_FORMAT, QVariant(static_cast<int>(m_nDisplayFormat)));

  settings.setValue(S_RX_AUTOSTART, m_autoStartRx);
  settings.setValue(S_RX_UNIVERSESLIST_START, m_universesListStart);
  settings.setValue(S_RX_UNIVERSESLIST_COUNT, m_universesListCount);

  settings.setValue(S_RX_BLIND_VISUALIZER, QVariant(m_bBlindVisualizer));
  settings.setValue(S_RX_ETC_DD, QVariant(m_bETCDD));
  settings.setValue(S_RX_ETC_DDONLY, QVariant(m_bETCDisplayDDOnly));
  settings.setValue(S_RX_FLICKERFINDERSHOWINFO, QVariant(m_flickerFinderShowInfo));
  
  settings.setValue(S_RX_BAD_PRIORITY, m_rxbadpriority);

  settings.setValue(S_TX_DEFAULT_SOURCENAME, m_sDefaultTransmitName);
  settings.setValue(S_TX_TIMEOUT, QVariant(m_nNumSecondsOfSacn));
  settings.setValue(S_TX_RATE_OVERRIDE, m_txrateoverride);
  settings.setValue(S_TX_BAD_PRIORITY, m_txbadpriority);

  settings.setValue(S_SAVEWINDOWLAYOUT, m_autosaveWindowLayout);
  settings.setValue(S_RESTOREWINDOWLAYOUT, m_restoreWindowLayout);

  saveWindowGeometrySettings();

  for (int i = 0; i < PRESET_COUNT; i++)
  {
    // Only store if not default
    const QString preset_name = S_TX_PRESETS.arg(i);
    if (settings.contains(preset_name) || m_presets[i] != DefaultByteArrayDmx)
      settings.setValue(preset_name, m_presets[i]);
  }

  for (int i = 0; i < PRIORITYPRESET_COUNT; i++)
  {
    // Only store if not default
    const QString preset_name = S_TX_PRIORITYPRESET.arg(i);
    if (settings.contains(preset_name) || m_priorityPresets[i] != DefaultByteArrayPriority(i))
      settings.setValue(preset_name, m_priorityPresets[i]);
  }

  settings.setValue(S_TX_MULTICAST_TTL, m_multicastTtl);

  settings.setValue(S_PATHWAYSECURE_RX, m_pathwaySecureRx);
  settings.setValue(S_PATHWAYSECURE_RX_DATA_ONLY, m_pathwaySecureRxDataOnly);
  settings.setValue(S_PATHWAYSECURE_RX_PASSWORD, m_pathwaySecureRxPassword);
  settings.setValue(S_PATHWAYSECURE_RX_SEQUENCE_TIME_WINDOW, m_pathwaySecureRxSequenceTimeWindow);
  settings.setValue(S_PATHWAYSECURE_TX_PASSWORD, m_pathwaySecureTxPassword);
  settings.setValue(S_PATHWAYSECURE_TX_SEQUENCE_TYPE, m_pathwaySecureTxSequenceType);
  settings.setValue(S_PATHWAYSECURE_TX_SEQUENCE_BOOT_COUNT, m_pathwaySecureTxSequenceBootCount);
  SaveByteArray(settings, S_PATHWAYSECURE_SEQUENCE_MAP, m_pathwaySecureSequenceMap);

  settings.sync();
}

QSettings Preferences::getSettings() const
{
  if (m_settings_file.isEmpty())
    return QSettings();
  return QSettings(m_settings_file, QSettings::IniFormat);
}

void Preferences::loadPreferences()
{
  QSettings settings = getSettings();

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
  m_bBlindVisualizer = settings.value(S_RX_BLIND_VISUALIZER, m_bBlindVisualizer).toBool();
  m_bETCDisplayDDOnly = settings.value(S_RX_ETC_DDONLY, m_bETCDisplayDDOnly).toBool();
  m_bETCDD = settings.value(S_RX_ETC_DD, m_bETCDD).toBool();
  m_sDefaultTransmitName = settings.value(S_TX_DEFAULT_SOURCENAME, m_sDefaultTransmitName).toString();
  m_nNumSecondsOfSacn = settings.value(S_TX_TIMEOUT, m_nNumSecondsOfSacn).toInt();
  m_flickerFinderShowInfo = settings.value(S_RX_FLICKERFINDERSHOWINFO, m_flickerFinderShowInfo).toBool();
  m_windowMode = static_cast<WindowMode>(settings.value(S_WINDOW_MODE, static_cast<int>(m_windowMode)).toInt());
  m_autosaveWindowLayout = settings.value(S_SAVEWINDOWLAYOUT, m_autosaveWindowLayout).toBool();
  m_restoreWindowLayout = settings.value(S_RESTOREWINDOWLAYOUT, m_restoreWindowLayout).toBool();
  m_autoStartRx = settings.value(S_RX_AUTOSTART, m_autoStartRx).toBool();
  m_theme = static_cast<Themes::theme_e>(settings.value(S_THEME, static_cast<int>(m_theme)).toInt());
  m_txrateoverride = settings.value(S_TX_RATE_OVERRIDE, m_txrateoverride).toBool();
  m_txbadpriority = settings.value(S_TX_BAD_PRIORITY, m_txbadpriority).toBool();
  m_rxbadpriority = settings.value(S_RX_BAD_PRIORITY, m_rxbadpriority).toBool();
  {
    QVariant v = settings.value(S_LOCALE, m_locale);
    if (v.typeId() == QMetaType::QLocale)
      m_locale = v.toLocale();
    else
      m_locale = QLocale(v.toString());
  }
  m_universesListStart = settings.value(S_RX_UNIVERSESLIST_START, m_universesListStart).toUInt();
  m_universesListCount = settings.value(S_RX_UNIVERSESLIST_COUNT, m_universesListCount).toUInt();
  m_multicastTtl = settings.value(S_TX_MULTICAST_TTL, m_multicastTtl).toUInt();
  m_pathwaySecureRx = settings.value(S_PATHWAYSECURE_RX, m_pathwaySecureRx).toBool();
  m_pathwaySecureRxPassword = settings.value(S_PATHWAYSECURE_RX_PASSWORD, m_pathwaySecureRxPassword).toString();
  m_pathwaySecureTxPassword = settings.value(S_PATHWAYSECURE_TX_PASSWORD, m_pathwaySecureTxPassword).toString();
  m_pathwaySecureRxDataOnly = settings.value(S_PATHWAYSECURE_RX_DATA_ONLY, m_pathwaySecureRxDataOnly).toBool();
  m_pathwaySecureTxSequenceType = settings.value(S_PATHWAYSECURE_TX_SEQUENCE_TYPE, m_pathwaySecureTxSequenceType).toUInt();
  m_pathwaySecureTxSequenceBootCount = settings.value(S_PATHWAYSECURE_TX_SEQUENCE_BOOT_COUNT, m_pathwaySecureTxSequenceBootCount).toUInt();
  m_pathwaySecureRxSequenceTimeWindow = settings.value(S_PATHWAYSECURE_RX_SEQUENCE_TIME_WINDOW, m_pathwaySecureRxSequenceTimeWindow).toUInt();
  m_pathwaySecureSequenceMap = settings.value(S_PATHWAYSECURE_SEQUENCE_MAP, m_pathwaySecureSequenceMap).toByteArray();

  loadWindowGeometrySettings();

  for (int i = 0; i < PRESET_COUNT; i++)
  {
    const QString key = S_TX_PRESETS.arg(i);
    if (settings.contains(key))
    {
      // Never change the size
      m_presets[i].replace(0, MAX_DMX_ADDRESS, settings.value(key).toByteArray());
    }
  }

  for (int i = 0; i < PRIORITYPRESET_COUNT; i++)
  {
    const QString key = S_TX_PRIORITYPRESET.arg(i);
    if (settings.contains(key))
    {
      // Never change the size
      m_priorityPresets[i].replace(0, MAX_DMX_ADDRESS, settings.value(key).toByteArray());
    }
  }
}

void Preferences::loadWindowGeometrySettings()
{
  QSettings settings = getSettings();
  switch (m_windowMode)
  {
  default: break;
  case WindowMode::MDI: settings.beginGroup(S_GROUP_MDI_WINDOW); break;
  case WindowMode::Floating: settings.beginGroup(S_GROUP_FLOATING_WINDOW); break;
  }

  m_mainWindowGeometry.first = settings.value(S_WINDOWGEOM, m_mainWindowGeometry.first).toByteArray();
  m_mainWindowGeometry.second = settings.value(S_WINDOWSTATE, m_mainWindowGeometry.second).toByteArray();

  m_windowInfo.clear();
  int size = settings.beginReadArray(S_SUBWINDOWLIST);
  for (int i = 0; i < size; i++)
  {
    SubWindowInfo value;
    settings.setArrayIndex(i);
    value.name = settings.value(S_WINDOWNAME).toString();
    value.geometry = settings.value(S_WINDOWGEOM).toByteArray();
    value.config = settings.value(S_WINDOWCONFIG).toJsonObject();
    m_windowInfo << value;
  }
  settings.endArray();
}

void Preferences::saveWindowGeometrySettings() const
{
  QSettings settings = getSettings();

  settings.setValue(S_WINDOW_MODE, static_cast<int>(m_windowMode));

  switch (m_windowMode)
  {
  default: break;
  case WindowMode::MDI: settings.beginGroup(S_GROUP_MDI_WINDOW); break;
  case WindowMode::Floating: settings.beginGroup(S_GROUP_FLOATING_WINDOW); break;
  }

  SaveByteArray(settings, S_WINDOWGEOM, m_mainWindowGeometry.first);
  SaveByteArray(settings, S_WINDOWSTATE, m_mainWindowGeometry.second);

  settings.beginWriteArray(S_SUBWINDOWLIST);
  for (int i = 0; i < m_windowInfo.count(); i++)
  {
    settings.setArrayIndex(i);
    const SubWindowInfo& info = m_windowInfo[i];
    settings.setValue(S_WINDOWNAME, info.name);
    SaveByteArray(settings, S_WINDOWGEOM, info.geometry);
    if (info.config.isEmpty())
      settings.remove(S_WINDOWCONFIG);
    else
      settings.setValue(S_WINDOWCONFIG, info.config);
  }
  settings.endArray();
}
