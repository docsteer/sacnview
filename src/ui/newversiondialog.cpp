#include "newversiondialog.h"
#include "ui_newversiondialog.h"
#include <QStandardPaths>
#include <QProcess>
#include <QStringLiteral>
#include "preferences.h"

#ifdef Q_OS_WIN
const QString OS_FILE_IDENTIFIER = QStringLiteral(".exe");
const QString OS_EXE_HANDLER = QStringLiteral("");
#endif

#ifdef Q_OS_MACOS
const QString OS_FILE_IDENTIFIER = QStringLiteral(".dmg");
const QString OS_EXE_HANDLER = QStringLiteral("open");
#endif

#ifdef Q_OS_LINUX
const QString OS_FILE_IDENTIFIER = QStringLiteral(".deb");
const QString OS_EXE_HANDLER = QStringLiteral("xdg-open");
#endif


// Enable to force the version on Github to be considered newer
//#define DEBUG_VERSIONCHECK

NewVersionDialog::NewVersionDialog(QWidget *parent) : QDialog(parent), ui(new Ui::NewVersionDialog)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    #if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
        setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    #endif
    connect(ui->btnLater, SIGNAL(pressed()), this, SLOT(reject()));
    m_manager = new QNetworkAccessManager(this);

    ui->btnInstall->setDefault(true);
}

void NewVersionDialog::setNewVersionNumber(const QString &version)
{
    ui->lblVersion->setText(tr("sACNView %1 is available (you have %2). Would you like to download it now?")
                            .arg(version, VERSION));
    m_newVersion = version;
}

void NewVersionDialog::setDownloadUrl(const QString &url)
{
    m_dlUrl = url;
}

void NewVersionDialog::on_btnInstall_pressed()
{
    ui->lblDownloadInfo->setText(tr("Downloading %1 from %2")
                                 .arg(m_newVersion, m_dlUrl));
    ui->stackedWidget->setCurrentIndex(1);
    this->adjustSize();
    ui->btnExitInstall->setEnabled(false);
    ui->progressBar->setValue(0);

    m_storagePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QUrl url(m_dlUrl);
    m_storagePath += QString("/");
    m_storagePath += url.path().split(QChar('/')).last();

    doDownload(url);
}

void NewVersionDialog::doDownload(const QUrl &url)
{
    // Begin the download
    if(m_dlFile.isOpen())
        m_dlFile.close();

    m_dlFile.setFileName(m_storagePath);
    bool ok = m_dlFile.open(QIODevice::WriteOnly);
    if(!ok)
    {
        ui->lblDownloadInfo->setText(
              tr("Could not open file %1 to save - please download and install manually").arg(m_storagePath));
        return;
    }

    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(dataReadyRead()));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
}

void NewVersionDialog::progress(qint64 bytes, qint64 total)
{
    if (!total) return;
    auto percent = static_cast<int>((bytes * 100) / total);
    ui->progressBar->setMaximum(100);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setValue(percent);

    #if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
        auto strProgress = tr("%1 of %2 bytes").arg(bytes).arg(total);
    #else
        auto strProgress = tr("%1 of %2")
                .arg(
                    this->locale().formattedDataSize(bytes),
                    this->locale().formattedDataSize(total)
                    );
    #endif

    ui->progressBar->setTextVisible(true);
    ui->progressBar->setFormat(strProgress);
    ui->progressBar->setAlignment(Qt::AlignCenter);
}

void NewVersionDialog::finished()
{
    QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
    if(!reply) return;

    // Github redirects to a CDN, so we check for that:
    QVariant possibleRedirectUrl =
                 reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if(!possibleRedirectUrl.toString().isEmpty())
    {
        ui->progressBar->setValue(0);
        QUrl redirUrl(possibleRedirectUrl.toString());
        doDownload(redirUrl);
        return;
    }

    if(reply->error() == QNetworkReply::NoError)
    {
        ui->btnCancelDl->setEnabled(false);
        ui->btnExitInstall->setEnabled(true);
        if(m_dlFile.isOpen())
            m_dlFile.close();
    }
    else
    {
        ui->lblDownloadInfo->setText(tr("Error downloading : please try again"));
    }

}

void NewVersionDialog::dataReadyRead()
{
    QNetworkReply *reply = dynamic_cast<QNetworkReply *>(sender());
    if(!reply) return;
    QByteArray data = reply->readAll();
    m_dlFile.write(data);
}

void NewVersionDialog::setNewVersionInfo(const QString &info)
{
    ui->teReleaseNotes->clear();
    ui->teReleaseNotes->setPlainText(info);
}

void NewVersionDialog::on_btnExitInstall_pressed()
{
    bool ok = false;
    if (!OS_EXE_HANDLER.isEmpty())
        ok = QProcess::startDetached(OS_EXE_HANDLER, QStringList(m_storagePath));
    else
        ok = QProcess::startDetached(m_storagePath, QStringList());

    if(!ok)
        QMessageBox::warning(this, tr("Couldn't Run Installer"), tr("Unable to run installer - please run %1").arg(m_storagePath));
    qApp->exit();
}

void NewVersionDialog::on_btnCancelDl_pressed()
{
    reject();
}

void NewVersionDialog::on_btnIgnore_pressed()
{
    reject();
    qDebug() << "[Version check] Requested to ignore" << m_newVersion;
    Preferences::getInstance()->SetUpdateIgnore(m_newVersion);
}

VersionCheck::VersionCheck(QObject *parent):
    QObject(parent)
{
    manager = new QNetworkAccessManager(this);
    QNetworkRequest request;

    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    request.setRawHeader("User-Agent", QString("%1 %2").arg(APP_NAME, VERSION).toUtf8());
    request.setRawHeader("Accept", "application/vnd.github.v3.raw+json");
    request.setUrl(QUrl("https://api.github.com/repos/docsteer/sacnview/releases"));

    manager->get(request);
}

void VersionCheck::replyFinished (QNetworkReply *reply)
{
    QString downloadUrl;
    if(reply->error())
    {
        qDebug() << "[Version check] Unable to check for updated version: " << reply->errorString();
        switch (reply->error()) {
            case QNetworkReply::HostNotFoundError:
            case QNetworkReply::TimeoutError:
                break;
            default:
                QMessageBox::information(nullptr,
                    tr("Version Check"),
                    tr("Error %1\r\n%2").arg(QString::number(reply->error()), reply->errorString()));
                break;
        }
    }
    else
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(reply->readAll());
        if (jDoc.isNull()) {
            qDebug() << "[Version check] Response not valid JSON";
        } else {
            QLocale locale("US");

            QDate myDate = locale.toDate(
                        QString("%1 %2 %3").arg(GIT_DATE_DATE, GIT_DATE_MONTH, GIT_DATE_YEAR),
                        QString("dd MMM yyyy")
                        );
            qDebug() << "[Version check] My version:" << VERSION << myDate.toString() <<  "Pre Release - " << PRERELEASE;

            QJsonArray jArray = jDoc.array();
            for (const QJsonValue &jValue : jArray) {
                QJsonObject jObj = jValue.toObject();

                QDate objectDate = locale.toDateTime(
                            jObj["published_at"].toString(),
                            QString("yyyy-MM-dd'T'hh':'mm':'ss'Z'")
                            ).date();
                QString remote_version = jObj["tag_name"].toString();
                bool remote_is_prerelease = jObj["prerelease"].toBool();

#ifdef DEBUG_VERSIONCHECK
                objectDate = QDate(2199, 12, 12);
                remote_version = QString("AwesomeVersion");
#endif

                qDebug() << "[Version check] Remote version:" << remote_version << objectDate.toString() << "Pre Release - " << remote_is_prerelease ;

                // Find the appropriate download URL for the platform
                QJsonArray assetsArray = jObj["assets"].toArray();
                for (const QJsonValue &asset : assetsArray)
                {
                    QJsonObject oAsset = asset.toObject();

                    if(oAsset.contains("browser_download_url"))
                    {
                        QString url = oAsset.value("browser_download_url").toString();
                        if(url.contains(OS_FILE_IDENTIFIER))
                        {
                            downloadUrl = url;
                        }
                    }

                }

                if (myDate.isValid() && objectDate.isValid()) {
                    if (VERSION != remote_version) {
                        if (objectDate > myDate) {
                            if (PRERELEASE || !remote_is_prerelease) { // If prerelease, always upgrade; if not, only upgrade to non-prerelease
                                // Newer!
                                qDebug() << "[Version check] Remote version" << remote_version << "is newer!";

                                // Has the user asked us to ingore this version?
                                if (Preferences::getInstance()->GetUpdateIgnore() == remote_version)
                                {
                                    qDebug() << "[Version check] Requested to ignore version" << remote_version;
                                } else {
                                    // Tell the user
                                    NewVersionDialog dlg;
                                    dlg.setNewVersionNumber(remote_version);
                                    dlg.setNewVersionInfo(jObj["body"].toString());
                                    dlg.setDownloadUrl(downloadUrl);
                                    dlg.exec();
                                }
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    reply->deleteLater();
}
