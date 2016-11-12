#ifndef MERGEDUNIVERSELOGGER_H
#define MERGEDUNIVERSELOGGER_H

#include "sacnlistener.h"
#include <QFile>
#include <QTextStream>
#include <QObject>

class MergedUniverseLogger : public QObject
{
    Q_OBJECT

public:
    MergedUniverseLogger();

public slots:
    void start(QString fileName, sACNListener *listener);
    void stop();

    void levelsChanged();

private:
    void setUpFile(QString fileName);
    void closeFile();

    QElapsedTimer m_elapsedTimer;
    QFile * m_file;
    QTextStream * m_stream;
    sACNListener * m_listener;
};

#endif // MERGEDUNIVERSELOGGER_H
