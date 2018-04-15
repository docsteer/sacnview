#include "logwindow.h"
#include "ui_logwindow.h"

#include "consts.h"
#include <QDateTime>
#include <QClipboard>
#include <QScrollBar>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

LogWindow::LogWindow(int universe, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogWindow)
{
    ui->setupUi(this);

    /* Set universe */
    setWindowTitle(tr("Log - Universe %1").arg(universe));
    m_listener = sACNManager::getInstance()->getListener(universe);

    /* Time and Date formats */
    for(const auto &item : lTimeFormat)
    {
        ui->cbTimeFormat->addItem(item.friendlyName);
    }

    /* Display formats */
    for(const auto &item : lDisplayFormat)
    {
        ui->cbDisplayFormat->addItem(item.friendlyName);
    }

    /* Level format */
    // Byte
    ui->cbLevelFormat->addItem(
                tr("Byte"),
                eLevelFormat::levelFormatByte);
    // Percent
    ui->cbLevelFormat->addItem(
                tr("Percent"),
                eLevelFormat::levelFormatPercent);
    // Hex
    ui->cbLevelFormat->addItem(
                tr("Hex"),
                eLevelFormat::levelFormatHex);

    // Setup callbacks
    on_cbLevels_clicked(ui->cbLevels->isChecked());
    on_cbSources_clicked(ui->cbSources->isChecked());
}

LogWindow::~LogWindow()
{
    closeLogFile();
    delete ui;
}

void LogWindow::appendLogLine(QString &line) {

    // Prepend Time and date
    if (lTimeFormat[ui->cbTimeFormat->currentIndex()].strFormat.isEmpty()) {
        auto timeFormat = lTimeFormat[ui->cbTimeFormat->currentIndex()].dateFormat;
        line.prepend(QString("%1: ").arg(QDateTime::currentDateTime().toString(timeFormat)));
    } else {
        auto timeFormat = lTimeFormat[ui->cbTimeFormat->currentIndex()].strFormat;
        line.prepend(QString("%1: ").arg(QDateTime::currentDateTime().toString(timeFormat)));
    }


    // Log to window
    if (ui->cbLogToWindow->isChecked())
    {
        ui->lvLog->addItem(line);

        // To many lines?
        if (ui->lvLog->count() > ui->sbLogtoWindowLimit->value())
        {
            auto removeCount = ui->lvLog->count() - ui->sbLogtoWindowLimit->value();
            ui->lvLog->model()->removeRows(0,removeCount);
        }
    }

    // Log to file
    if (m_fileStream && ui->cbLogToFile->isChecked())
    {
        *m_fileStream << line << endl;
    }
}

void LogWindow::onLevelsChanged()
{
    const QString levelFormat = lDisplayFormat[ui->cbDisplayFormat->currentIndex()].format;
    const bool onlyChanged = ui->cbOnlyChanged->isEnabled() && ui->cbOnlyChanged->isChecked();
    const QChar seperator = lDisplayFormat[ui->cbDisplayFormat->currentIndex()].seperator;
    const sACNMergedSourceList list = m_listener->mergedLevels();

    QString logLine;
    logLine.reserve(MAX_DMX_ADDRESS * levelFormat.size());
    for(int i=0; i<list.count(); i++)
    {
        sACNMergedAddress a = list.at(i);

        // Only display changed?
        if (onlyChanged && !a.changedSinceLastMerge)
            continue;

        // Format level value
        QString levelValue;
        switch (ui->cbLevelFormat->currentData().toInt()) {
            default:
            case eLevelFormat::levelFormatByte:
                levelValue = QString::number(a.level);
            break;
            case eLevelFormat::levelFormatPercent:
                {
                    quint8 percent = ((double)a.level / std::numeric_limits<uchar>::max() ) * 100;
                    levelValue = QString::number(percent);
                }
            break;
            case eLevelFormat::levelFormatHex:
                levelValue = QString::number(a.level, 16);
            break;
        }

        // Format chan and/or level display
        QString levelData = levelFormat;
        levelData.replace("{CHAN}", QString::number((i+1)));
        levelData.replace("{LEVEL}", levelValue);

        // Seperator
        if (i != list.count() - 1)
            levelData.append(seperator);

        // Write
        logLine.append(levelData);
    }

    appendLogLine(logLine);
}

void LogWindow::onSourceFound(sACNSource *source) {
    QString logLine = tr("Source Found: %1 (%2)")
            .arg(source->name)
            .arg(source->ip.toString());

    appendLogLine(logLine);
}

void LogWindow::onSourceResume(sACNSource *source) {
    QString logLine = tr("Source Resumed: %1 (%2)")
            .arg(source->name)
            .arg(source->ip.toString());

    appendLogLine(logLine);
}

void LogWindow::onSourceLost(sACNSource *source) {
    QString logLine = tr("Source Lost: %1 (%2)")
            .arg(source->name)
            .arg(source->ip.toString());

    appendLogLine(logLine);
}

void LogWindow::on_btnCopyClipboard_pressed()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QStringList list;
    for (  auto item : ui->lvLog->selectionModel()->selectedIndexes() ) {
        list << item.data().toString();
    }
    clipboard->setText(list.join("\n"));
}

void LogWindow::on_btnClear_pressed()
{
    ui->lvLog->clear();
}

void LogWindow::on_cbLevels_clicked(bool checked)
{
    if (checked) {
        connect(m_listener.data(), SIGNAL(levelsChanged()), this, SLOT(onLevelsChanged()));
    } else {
        disconnect(m_listener.data(), SIGNAL(levelsChanged()), this, SLOT(onLevelsChanged()));
    }
}

void LogWindow::on_cbSources_clicked(bool checked)
{
    if (checked)
    {
        connect(m_listener.data(), SIGNAL(sourceFound(sACNSource*)), this, SLOT(onSourceFound(sACNSource*)));
        connect(m_listener.data(), SIGNAL(sourceResumed(sACNSource*)), this, SLOT(onSourceResume(sACNSource*)));
        connect(m_listener.data(), SIGNAL(sourceLost(sACNSource*)), this, SLOT(onSourceLost(sACNSource*)));
    } else {
        disconnect(m_listener.data(), SIGNAL(sourceFound(sACNSource*)), this, SLOT(onSourceFound(sACNSource*)));
        disconnect(m_listener.data(), SIGNAL(sourceResumed(sACNSource*)), this, SLOT(onSourceResume(sACNSource*)));
        disconnect(m_listener.data(), SIGNAL(sourceLost(sACNSource*)), this, SLOT(onSourceLost(sACNSource*)));
    }
}

void LogWindow::on_cbDisplayFormat_currentIndexChanged(int index)
{
    ui->cbOnlyChanged->setEnabled(lDisplayFormat[index].onlyChangedAllowed);
    ui->cbOnlyChanged->setChecked(lDisplayFormat[index].onlyChangedAllowed);
}

bool LogWindow::openLogFile() {
    closeLogFile();

    // Setup dialog box
    QString fileName;
    QFileDialog dialog(this);
    dialog.setWindowTitle(APP_NAME);
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    dialog.setNameFilter("Log Files (*.log)");
    dialog.setDefaultSuffix("log");
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if(dialog.exec())
    {
        fileName = dialog.selectedFiles().at(0);
        if(fileName.isEmpty()) {
            return false;
        }
    } else {
        return false;
    }

    Q_ASSERT(m_file == nullptr);

    m_file = new QFile(fileName);
    if (m_file->open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        m_fileStream = new QTextStream(m_file);
        if (m_fileStream)
        {
            QFileInfo fileInfo(m_file->fileName());
            ui->lblLogToFile->setText(fileInfo.fileName());
            ui->lblLogToFile->setToolTip(QDir::toNativeSeparators(m_file->fileName()));
            return true;
        }
    }

    // Failed and not canceled...
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QObject::tr("Failed to start logging to file\r\nError %1").arg(m_file->error()));
    msgBox.exec();

    return false;
}

void LogWindow::closeLogFile() {
    delete m_fileStream;
    delete m_file;

    m_fileStream = nullptr;
    m_file = nullptr;

    ui->lblLogToFile->setText(QString());
    ui->lblLogToFile->setToolTip(QString());
}

void LogWindow::on_cbLogToFile_clicked(bool checked)
{
    if (checked & !m_fileStream)
    {
        on_pbLogToFile_clicked();
    }

    ui->pbLogToFile->setEnabled(!ui->cbLogToFile->isChecked());
}

void LogWindow::on_pbLogToFile_clicked()
{
    if (!openLogFile())
    {
        ui->cbLogToFile->setChecked(false);

        closeLogFile();
    }
}
