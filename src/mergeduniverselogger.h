// Copyright (c) 2016 Hans Hinrichsen for the sACNView project
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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

private slots:
    void levelsChanged();

private:
    void setUpFile(QString fileName);
    void closeFile();

    QString m_stringBuffer;
    QElapsedTimer m_elapsedTimer;
    QFile * m_file;
    QTextStream * m_stream;
    sACNListener * m_listener;
};

#endif // MERGEDUNIVERSELOGGER_H
