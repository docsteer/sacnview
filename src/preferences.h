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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "CID.h"
#include "consts.h"
#include "themes/themes.h"

#include <QObject>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QColor>
#include <QLocale>

#include <array>

struct SubWindowInfo
{
  QString name;
  QByteArray geometry;
};

class Preferences
{

public:
  enum class Status
  {
    Good,
    Warning,
    Bad
  };

private:
  explicit Preferences();
public:
  ~Preferences();

  /**
   * @brief Instance - returns the instance of the Preferences class
   * @return the instance
   */
  static Preferences& Instance();

  /**
   * @brief networkInterface returns the user preferred network interface for multicast
   * @return the network interface to use
   */
  QNetworkInterface networkInterface() const;
  Q_SLOT void setNetworkInterface(const QNetworkInterface& value);

  /**
 * @brief interfaceSuitable - returns whether the interface is suitable for sACN
 * @param inter - the interface to check
 * @return true/false
 */
  static bool interfaceSuitable(const QNetworkInterface& iface);

  /**
   * @brief defaultInterfaceAvailable - returns whether the default interface selected by the user
   * is available
   * @return true/false
   */
  bool defaultInterfaceAvailable() const { return interfaceSuitable(m_interface); }

  static QString GetIPv4AddressString(const QNetworkInterface& inter);

  QColor colorForCID(const CID& cid) const;
  QColor colorForStatus(Status status) const;

  // Preferences access functions here:
  void SetDisplayFormat(DisplayFormat nDisplayFormat) { m_nDisplayFormat = nDisplayFormat; }
  DisplayFormat GetDisplayFormat() const { return m_nDisplayFormat; }
  unsigned int GetMaxLevel() const { return (m_nDisplayFormat == DisplayFormat::PERCENT) ? 100u : MAX_SACN_LEVEL; }

  void SetBlindVisualizer(bool bBlindVisualizer) { m_bBlindVisualizer = bBlindVisualizer; }
  bool GetBlindVisualizer() const { return m_bBlindVisualizer; }

  void SetETCDisplayDDOnly(bool bETCDDOnly) { m_bETCDisplayDDOnly = bETCDDOnly; }
  bool GetETCDisplayDDOnly() const { return m_bETCDisplayDDOnly; }

  void SetETCDD(bool bETCDD) { m_bETCDD = bETCDD; }
  bool GetETCDD() const { return m_bETCDD; }

  void SetDefaultTransmitName(const QString& sDefaultTransmitName);
  const QString& GetDefaultTransmitName() const { return m_sDefaultTransmitName; }

  void SetNumSecondsOfSacn(int nNumSecondsOfSacn);
  unsigned int GetNumSecondsOfSacn() const { return m_nNumSecondsOfSacn; }

  void setFlickerFinderShowInfo(bool showIt) { m_flickerFinderShowInfo = showIt; }
  bool getFlickerFinderShowInfo() const { return m_flickerFinderShowInfo; }

  void SetPreset(const QByteArray& data, int index);
  const QByteArray& GetPreset(int index) const;

  void SetSaveWindowLayout(bool value) { m_saveWindowLayout = value; }
  bool GetSaveWindowLayout() const { return m_saveWindowLayout; }

  void SetWindowMode(WindowMode mode);
  WindowMode GetWindowMode() const { return m_windowMode; }

  void SetMainWindowGeometry(const QByteArray& geo, const QByteArray& state) { m_mainWindowGeometry = { geo, state }; }
  const QByteArray& GetMainWindowGeometry() const { return m_mainWindowGeometry.first; }
  const QByteArray& GetMainWindowState() const { return m_mainWindowGeometry.second; }

  void SetSavedWindows(const QList<SubWindowInfo>& values) { m_windowInfo = values; }
  const QList<SubWindowInfo>& GetSavedWindows() const { return m_windowInfo; }

  void SetNetworkListenAll(const bool& value);
  bool GetNetworkListenAll() const;

  void SetTheme(Themes::theme_e theme) { m_theme = theme; }
  Themes::theme_e GetTheme() const { return m_theme; }

  void SetTXRateOverride(bool override) { m_txrateoverride = override; }
  bool GetTXRateOverride() const { return m_txrateoverride; }

  void SetLocale(const QLocale& locale) { m_locale = locale; }
  const QLocale& GetLocale() const { return m_locale; }

  void SetUniversesListed(quint8 count) { m_universesListed = (std::max)(count, (quint8)1); }
  quint8 GetUniversesListed() const { return m_universesListed; }

  void SetPriorityPreset(const QByteArray& data, int index);
  const QByteArray& GetPriorityPreset(int index) const;

  void SetMulticastTtl(quint8 ttl) { m_multicastTtl = ttl; }
  quint8 GetMulticastTtl() const { return m_multicastTtl; }

  void SetPathwaySecureRx(bool enable) { m_pathwaySecureRx = enable; }
  bool GetPathwaySecureRx() const { return m_pathwaySecureRx; }

  void SetPathwaySecureRxPassword(const QString& password) { m_pathwaySecureRxPassword = password; }
  const QString& GetPathwaySecureRxPassword() const { return m_pathwaySecureRxPassword; }

  void SetPathwaySecureTxPassword(const QString& password) { m_pathwaySecureTxPassword = password; }
  const QString& GetPathwaySecureTxPassword() const { return m_pathwaySecureTxPassword; }

  void SetPathwaySecureRxDataOnly(bool value) { m_pathwaySecureRxDataOnly = value; }
  bool GetPathwaySecureRxDataOnly() const { return m_pathwaySecureRxDataOnly; }

  void SetPathwaySecureTxSequenceType(quint8 type) { m_pathwaySecureTxSequenceType = type; }
  quint8 GetPathwaySecureTxSequenceType() const { return m_pathwaySecureTxSequenceType; }

  void SetPathwaySecureRxSequenceTimeWindow(quint32 value) { m_pathwaySecureRxSequenceTimeWindow = value; }
  quint32 GetPathwaySecureRxSequenceTimeWindow() const { return m_pathwaySecureRxSequenceTimeWindow; }

  void SetPathwaySecureRxSequenceBootCount(quint32 value) { m_pathwaySecureTxSequenceBootCount = value; }
  quint32 GetPathwaySecureRxSequenceBootCount() const { return m_pathwaySecureTxSequenceBootCount; }

  void SetPathwaySecureSequenceMap(const QByteArray& map);
  QByteArray GetPathwaySecureSequenceMap() const;

  void SetUpdateIgnore(const QString& version);
  QString GetUpdateIgnore() const;

  bool GetRestartPending() const { return m_restartPending; }
  void SetRestartPending() { m_restartPending = true; }

  QString GetFormattedValue(unsigned int nLevelInDecimal, bool decorated = false) const;

  void savePreferences() const;

private:
  QNetworkInterface m_interface;
  mutable QHash<CID, QColor> m_cidToColor;

  QString m_sDefaultTransmitName = DEFAULT_SOURCE_NAME;

  // Window geometries
  WindowMode m_windowMode = WindowMode::MDI;
  std::pair<QByteArray, QByteArray> m_mainWindowGeometry;
  QList<SubWindowInfo> m_windowInfo;

  DisplayFormat m_nDisplayFormat = DisplayFormat::DECIMAL;
  unsigned int m_nNumSecondsOfSacn = 0;

  std::array<QByteArray, PRESET_COUNT> m_presets;
  std::array<QByteArray, PRIORITYPRESET_COUNT> m_priorityPresets;

  Themes::theme_e m_theme = Themes::LIGHT;

  QLocale m_locale;
  quint8 m_universesListed = 20;

  quint8 m_multicastTtl = 1;

  QString m_pathwaySecureRxPassword = QStringLiteral("Correct Horse Battery Staple");
  QString m_pathwaySecureTxPassword = m_pathwaySecureRxPassword;

  QByteArray m_pathwaySecureSequenceMap;
  quint8 m_pathwaySecureTxSequenceType = 0;
  quint32 m_pathwaySecureTxSequenceBootCount = 0;
  quint32 m_pathwaySecureRxSequenceTimeWindow = 1000;

  bool m_interfaceListenAll = false;
  bool m_flickerFinderShowInfo = true;
  bool m_bBlindVisualizer = true;
  bool m_bETCDisplayDDOnly = true;
  bool m_bETCDD = true;
  bool m_txrateoverride = false;

  bool m_pathwaySecureRx = true;
  bool m_pathwaySecureRxDataOnly = false;

  bool m_saveWindowLayout = false;

  bool m_restartPending = false;

  void loadPreferences();
  void loadWindowGeometrySettings();
  void saveWindowGeometrySettings() const;
};

#endif // PREFERENCES_H
