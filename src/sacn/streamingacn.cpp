// Copyright (c) 2015 Electronic Theatre Controls, http://www.etcconnect.com
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

#include "streamingacn.h"
#include "sacnlistener.h"

#include <QThread>

sACNSource::sACNSource()
{
    src_valid = false;  //We're ignoring thread issues, so to reuse a slot in the source array we'll flag it valid or invalid
    lastseq = 0;
    waited_for_dd = false;
    doing_dmx = false; //if true, we are processing dmx data from this source
    doing_per_channel = false;  //If true, we are tracking per-channel priority messages for this source
    isPreview = false;
    memset(level_array, -1, 512);
    memset(priority_array, -1, 512);
    priority = 0;
    fpsTimer.start();
    fpsCounter = 0;
    fps = 0;
    seqErr = 0;
    jumps = 0;
}

QString sACNSource::cid_string()
{
    char buffer[CID::CIDSTRINGBYTES];
    CID::CIDIntoString(this->src_cid, buffer);

    return QString(buffer);
}

sACNManager *sACNManager::m_instance = NULL;

sACNManager *sACNManager::getInstance()
{
    if(!m_instance)
        m_instance = new sACNManager();
    return m_instance;
}

sACNManager::sACNManager()
{

}

sACNListener *sACNManager::getListener(int universe)
{
    sACNListener *listener = NULL;
    if(!m_listenerHash.contains(universe))
    {
        listener = new sACNListener();
        QThread *newThread = new QThread();
        newThread->setObjectName(QString("Universe %1 RX").arg(universe));
        listener->moveToThread(newThread);
        newThread->start(QThread::HighPriority);
        m_listenerThreads[universe]  = newThread;
        QMetaObject::invokeMethod(listener,"startReception", Q_ARG(int,universe));
        m_listenerHash[universe] = listener;
    }
    else
        listener = m_listenerHash[universe];
    return listener;
}
