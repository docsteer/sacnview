#ifndef VERSIONCHECK_H
#define VERSIONCHECK_H

#include "consts.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDate>
#include <QLocale>
#include <QMessageBox>
#include <QDialog>

namespace Ui {
  class NewVersionDialog;
}

class NewVersionDialog : public QDialog
{
  Q_OBJECT
public:
  NewVersionDialog(QWidget* parent = Q_NULLPTR);

  void setNewVersionNumber(const QString& version);
  void setNewVersionInfo(const QString& info);
  void setDownloadUrl(const QString& url);
private slots:
  void on_btnInstall_pressed();
  void on_btnExitInstall_pressed();
  void on_btnCancelDl_pressed();
  void on_btnIgnore_pressed();
  void progress(qint64 bytes, qint64 total);
  void finished();
  void dataReadyRead();
private:
  Ui::NewVersionDialog* ui = Q_NULLPTR;
  QString m_dlUrl;
  QString m_newVersion;
  QNetworkAccessManager* m_manager = Q_NULLPTR;
  QNetworkReply* m_reply = Q_NULLPTR;
  QFile m_dlFile;
  QString m_storagePath;
  void doDownload(const QUrl& url);
};

class VersionCheck : public QObject
{
  Q_OBJECT
public:
  VersionCheck(QObject* parent = Q_NULLPTR);

public slots:
  void checkForUpdate();
  void replyFinished(QNetworkReply* reply);

private:
  QNetworkAccessManager* manager = Q_NULLPTR;
};

#endif // VERSIONCHECK_H
