// Copyright 2016 Tom Barthel-Steer
// http://www.tomsteer.net
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Parts of this file from Electronic Theatre Controls Inc, License info below
//
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

#include <QCoreApplication>
#include <QThread>
#include <QSharedPointer>
#include <QMessageBox>

sACNSource::sACNSource()
{
    src_valid = false;
    lastseq = 0;
    waited_for_dd = false;
    doing_dmx = false; //if true, we are processing dmx data from this source
    doing_per_channel = false;  //If true, we are tracking per-channel priority messages for this source
    isPreview = false;
    universe = 0;
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

sACNManager::sACNManager() : QObject()
{

}

static void strongPointerDelete(sACNListener *obj)
{
    sACNManager::getInstance()->listenerDelete(obj);
}

QSharedPointer<sACNListener> sACNManager::getListener(int universe)
{
    QMutexLocker locker(&sACNManager_mutex);
    // Notes on the memory management of sACNListeners :
    // This function creates a QSharedPointer to the listener, which is handed to the classes that
    // want to use it. It stores a QWeakPointer, which gets set to null when all instances of the shared
    // pointer are gone


    QSharedPointer<sACNListener> strongPointer;
    if(!m_listenerHash.contains(universe))
    {
        // Create listener as thread
        sACNListener *listener = Q_NULLPTR;
        listener = new sACNListener();
        listener->start(QThread::HighPriority);
        listener->setObjectName(QString("Universe %1 RX").arg(universe));
        m_listenerThreads[universe] = listener;

        // Start reception
        QMetaObject::invokeMethod(listener,"startReception", Q_ARG(int,universe));

        // Create strong pointer to return
        strongPointer = QSharedPointer<sACNListener>(listener, strongPointerDelete);
        m_listenerHash[universe] = strongPointer.toWeakRef();
        m_objToUniverse[listener] = universe;

        qDebug() << "Creating Listener for universe " << universe;
    }
    else
    {
        strongPointer = m_listenerHash[universe].toStrongRef();
    }
    if(strongPointer.isNull())
    {
        QMessageBox msgBox;
        msgBox.setText("Unable to allocate listener object\r\n\r\nsACNView must close now");
        msgBox.exec();
        qApp->exit(-1);
    }
    return strongPointer;
}

void sACNManager::listenerDelete(QObject *obj)
{
    QMutexLocker locker(&sACNManager_mutex);
    int universe = m_objToUniverse[obj];

    qDebug() << "Destroying Listener for universe " << universe;

    m_listenerHash.remove(universe);

    m_objToUniverse.remove(obj);

    m_listenerThreads[universe]->exit();
    m_listenerThreads[universe]->wait();
    m_listenerThreads[universe]->deleteLater();
    m_listenerThreads.remove(universe);
}
