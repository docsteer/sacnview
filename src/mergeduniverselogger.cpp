#include "mergeduniverselogger.h"

#include <QDateTime>

MergedUniverseLogger::MergedUniverseLogger() :
    m_file(nullptr), QObject(nullptr)
{
    //reserve a bit more than max length for sACNView1 format - see levelsChanged()
    m_stringBuffer.reserve(3000);
}

MergedUniverseLogger::~MergedUniverseLogger()
{
    //listener memory is dealt with externally
    closeFile();
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
    disconnect(m_listener, 0, this, 0);
    closeFile();
}

void MergedUniverseLogger::levelsChanged()
{
    //Must have valid stream and listener set up via start()
    //Also possible that this is getting called as we're wrapping up in stop()
    if(!m_stream || !m_listener) {
        return;
    }

    //Following sACNView1 format
    //MM/DD/YYYY HH:mm:SS AP,0.0000,[0,]x512

    //log levels to file
    auto levels = m_listener->mergedLevels();
    m_stringBuffer.clear();
    m_stringBuffer.append(QDateTime::currentDateTime().toString("M/d/yyyy h:mm:ss AP,"));
    m_stringBuffer.append(QString::number((double)m_elapsedTimer.elapsed()/1000, 'f', 4));
    for(auto level : levels) {
        m_stringBuffer.append(',');
        m_stringBuffer.append(QString::number(level.level));
    }
    m_stringBuffer.append('\n');
    *m_stream << m_stringBuffer;
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
    //finish out
    m_stream->flush();

    if(m_file->isOpen()) {
        m_file->close();
    }
    delete m_stream;
    m_file->deleteLater();

    m_stream = nullptr;
    m_file = nullptr;
}
