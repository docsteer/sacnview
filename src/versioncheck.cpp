#include "versioncheck.h"


VersionCheck::VersionCheck(QObject *parent):
    QObject(parent)
{
    manager = new QNetworkAccessManager(this);
    QNetworkRequest request;

    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    request.setRawHeader("User-Agent", QString("%1 %2").arg(APP_NAME).arg(VERSION).toUtf8());
    request.setRawHeader("Accept", "application/vnd.github.v3.raw+json");
    request.setUrl(QUrl("https://api.github.com/repos/docsteer/sacnview/releases"));

    manager->get(request);
}

void VersionCheck::replyFinished (QNetworkReply *reply)
{
    if(reply->error())
    {
        qDebug() << "[Version check] Unable to check for updated version: " << reply->errorString();
    }
    else
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(reply->readAll());
        if (jDoc.isNull()) {
            qDebug() << "[Version check] Response not valid JSON";
        } else {
            QLocale locale("US");

            QDate myDate = locale.toDate(
                        QString("%1 %2 %3").arg(GIT_DATE_DATE).arg(GIT_DATE_MONTH).arg(GIT_DATE_YEAR),
                        QString("dd MMM yyyy")
                        );
            qDebug() << "[Version check] My version:" << VERSION << myDate.toString();

            QJsonArray jArray = jDoc.array();
            foreach (const QJsonValue &jValue, jArray) {
                QJsonObject jObj = jValue.toObject();

                QDate objectDate = locale.toDateTime(
                            jObj["published_at"].toString(),
                            QString("yyyy-MM-dd'T'hh':'mm':'ss'Z'")
                            ).date();
                qDebug() << "[Version check] Remote version:" << jObj["tag_name"].toString() << objectDate.toString();

                if (myDate.isValid() && objectDate.isValid()) {
                    if (VERSION != jObj["tag_name"].toString()) {
                        if (objectDate > myDate) {
                            // Newer!
                            qDebug() << "[Version check] Remote version" << jObj["tag_name"].toString() << "is newer!";

                            // Tell the user
                            QMessageBox msgBox;
                            msgBox.setIcon(QMessageBox::Information);
                            msgBox.setStandardButtons(QMessageBox::Ok);
                            msgBox.setText(QObject::tr("A new version of sACNView is avaliable!"));
                            QString detailText = QObject::tr("This can be downloaded from:\nhttps://docsteer.github.io/sacnview/");
                            detailText.append(QString("\nInstalled: %1").arg(VERSION));
                            detailText.append(QString("\nAvailable: %2").arg(jObj["tag_name"].toString()));
                            msgBox.setDetailedText(detailText);
                            msgBox.exec();
                            return;
                        }
                    }
                }
            }
        }
    }
    reply->deleteLater();
}
