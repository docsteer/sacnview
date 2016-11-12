#include "mergeduniverselogger.h"

#include <QDateTime>

MergedUniverseLogger::MergedUniverseLogger() :
    m_file(nullptr), QObject(nullptr)
{

}

void MergedUniverseLogger::start(QString fileName, sACNListener *listener)
{
    m_elapsedTimer.start();
    setUpFile(fileName);
    m_listener = listener;
    connect(m_listener, &sACNListener::levelsChanged,
            this, &MergedUniverseLogger::levelsChanged);
}

void MergedUniverseLogger::stop()
{
    disconnect(m_listener);

    m_file->close();
}

void MergedUniverseLogger::levelsChanged()
{
    //MM/DD/YYYY HH:mm:SS AP,0.0000,0x512
    //log levels to file
    //header
    *m_stream << QDateTime::currentDateTime().toString("M/d/yyyy h:mm:ss AP,")
             << QString::number((double)m_elapsedTimer.elapsed()/1000, 'f', 4);
    //levels
    auto levels = m_listener->mergedLevels();
    for(auto level : levels) {
        *m_stream << ',' << level.level;
    }
    *m_stream << '\n';
}

void MergedUniverseLogger::setUpFile(QString fileName)
{
    Q_ASSERT(m_file == nullptr);

    m_file = new QFile(fileName);
    m_file->open(QIODevice::WriteOnly | QIODevice::Truncate);
    m_stream = new QTextStream(m_file);
}

void MergedUniverseLogger::closeFile()
{
    if(m_file->isOpen()) {
        m_file->close();
    }
    delete m_stream;
    m_file->deleteLater();

    m_stream = nullptr;
    m_file = nullptr;
}
