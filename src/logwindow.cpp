#include "logwindow.h"
#include "ui_logwindow.h"

#include <QDateTime>
#include <QClipboard>
#include <QScrollBar>

LogWindow::LogWindow(int universe, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogWindow)
{
    ui->setupUi(this);

    // Set universe
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
    delete ui;
}

void LogWindow::appendLogLine(QString line) {
    // Prepend Time and date
    if (lTimeFormat[ui->cbTimeFormat->currentIndex()].strFormat.isEmpty()) {
        auto timeFormat = lTimeFormat[ui->cbTimeFormat->currentIndex()].dateFormat;
        line.prepend(QString("%1: ").arg(QDateTime::currentDateTime().toString(timeFormat)));
    } else {
        auto timeFormat = lTimeFormat[ui->cbTimeFormat->currentIndex()].strFormat;
        line.prepend(QString("%1: ").arg(QDateTime::currentDateTime().toString(timeFormat)));
    }

    ui->teLog->appendPlainText(line);
    QScrollBar *sb = ui->teLog->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void LogWindow::onLevelsChanged()
{
    QString logLine;
    const QString levelFormat = lDisplayFormat[ui->cbDisplayFormat->currentIndex()].format;
    const bool onlyChanged = ui->cbOnlyChanged->isEnabled() && ui->cbOnlyChanged->isChecked();
    const QChar seperator = lDisplayFormat[ui->cbDisplayFormat->currentIndex()].seperator;
    const sACNMergedSourceList list = m_listener->mergedLevels();
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
    clipboard->setText(ui->teLog->toPlainText());
}

void LogWindow::on_btnClear_pressed()
{
    ui->teLog->clear();
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
    if (checked) {
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
