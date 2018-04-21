#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QWidget>
#include <QList>
#include "sacnlistener.h"

namespace Ui {
class LogWindow;
}

class LogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogWindow(int universe, QWidget *parent = 0);
    ~LogWindow();

private slots:
    void onLevelsChanged();
    void onSourceFound(sACNSource *source);
    void onSourceResume(sACNSource *source);
    void onSourceLost(sACNSource *source);

    void on_btnCopyClipboard_pressed();
    void on_btnClear_pressed();

    void on_cbLevels_clicked(bool checked);
    void on_cbSources_clicked(bool checked);

    void on_cbDisplayFormat_currentIndexChanged(int index);

    void on_cbLogToFile_clicked(bool checked);
    void on_pbLogToFile_clicked();

private:
    Ui::LogWindow *ui;

    QSharedPointer<sACNListener> m_listener;

    QFile *m_file;
    QTextStream *m_fileStream;
    void closeLogFile();
    bool openLogFile();

    void appendLogLine(QString &line);

    struct sTimeFormat {
        QString friendlyName;
        QString strFormat;
        Qt::DateFormat dateFormat;
    };
    QList<sTimeFormat> lTimeFormat = {
    #ifndef TARGET_WINXP
        {tr("ISO8601 w/Ms"), QString(), Qt::ISODateWithMs},
    #endif
        {tr("ISO8601"), QString(), Qt::ISODate},
        {tr("US 12Hour (sACNView 1)"), "M/d/yyyy h:mm:ss AP", (Qt::DateFormat)NULL},
        {tr("EU 12Hour"), "d/M/yyyy h:mm:ss AP", (Qt::DateFormat)NULL},
    };

    struct sDisplayFormat {
        QString friendlyName;
        QString format;
        QChar seperator;
        bool onlyChangedAllowed;
    };
    QList<sDisplayFormat> lDisplayFormat = {
        {tr("[Level1],[Levelx]...[Level512]"), "{LEVEL}", QChar(','), false},
        {tr("[Chan]@[Level]"), "{CHAN}@{LEVEL} ", QChar(' '), true},
    };

    enum eLevelFormat {
        levelFormatByte,
        levelFormatPercent,
        levelFormatHex
    };
};



#endif // LOGWINDOW_H
