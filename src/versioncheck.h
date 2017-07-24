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

class VersionCheck : public QObject
{
    Q_OBJECT
public:
    VersionCheck(QObject *parent = 0);

public slots:
    void replyFinished (QNetworkReply *reply);

private:
   QNetworkAccessManager *manager;
};

#endif // VERSIONCHECK_H
