// Copyright 2016 Tom Steer
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
#include "sacndiscovery.h"
#include "sacnlistener.h"
#include "sacnsender.h"

#include <QCoreApplication>
#include <QThread>

#ifdef QT_GUI_LIB
# include <QMessageBox>
#else
# include <QDebug>
#endif

QString GetProtocolVersionString(StreamingACNProtocolVersion value)
{
    switch (value)
    {
        case sACNProtocolDraft: return QObject::tr("Draft");
        case sACNProtocolRelease: return QObject::tr("Release");
        case sACNProtocolPathwaySecure: return QObject::tr("Pathway Secure");
        default: return QObject::tr("Unknown");
    }
}

sACNSource::sACNSource(const CID & source_cid, uint16_t universe)
    : src_cid(source_cid), universe(universe)
{}

void sACNSource::storeReceivedLevels(const uint8_t * pdata, uint16_t rx_slot_count)
{
    // Copy the last array back
    last_level_array = level_array;
    // Fill in the new array
    if (rx_slot_count < level_array.size()) level_array.fill(0);
    memcpy(level_array.data(), pdata, rx_slot_count);

    // Slot count change, re-merge all slots
    if (slot_count != rx_slot_count)
    {
        slot_count = rx_slot_count;
        source_params_change = true;
        source_levels_change = true;
        dirty_array.fill(true);
        return;
    }

    // Compare the two
    for (int i = 0; i < slot_count; ++i)
    {
        if (level_array[i] != last_level_array[i])
        {
            dirty_array[i] |= true;
            source_levels_change = true;
        }
    }
}

void sACNSource::storeReceivedPriorities(const uint8_t * pdata, uint16_t rx_slot_count)
{
    // Copy the last array back
    last_priority_array = priority_array;
    // Fill in the new array
    if (rx_slot_count < priority_array.size()) priority_array.fill(0);

    memcpy(priority_array.data(), pdata, rx_slot_count);

    // Compare the two and atomically update badness if acceptable.
    bool was_priority_array_bad = false;
    for (int i = 0; i < DMX_SLOT_MAX; i++)
    {
        if (priority_array[i] != last_priority_array[i])
        {
            dirty_array[i] |= true;
            source_levels_change = true;
        }
        if (priority_array[i] > MAX_SACN_PRIORITY) was_priority_array_bad = true;
    }

    priority_array_bad = was_priority_array_bad;
}

bool sACNSource::HasInvalidPriority() const
{
    return priority_array_bad || (priority > MAX_SACN_PRIORITY);
}

sACNManager & sACNManager::Instance()
{
    static sACNManager s_instance;
    return s_instance;
}

sACNManager::~sACNManager()
{
    QMutexLocker lock(&sACNManager_mutex);

    // Stop and delete all the threads
    for (QThread * thread : m_threadPool)
    {
        thread->exit();
        thread->wait();
        delete thread;
    }
    m_threadPool.clear();

    // Stop the Tock layer
    Tock_StopLib();
}

sACNManager::sACNManager()
    : QObject()
{
    // Start Tock layer
    Tock_StartLib();

    // Start E1.31 Universe Discovery
    sACNDiscoveryTX::start();
    sACNDiscoveryRX::start();

    // Create the threadpool
    const int threadCount = QThread::idealThreadCount();
    for (int i = 0; i < threadCount; ++i)
    {
        QThread * thread = new QThread(this);
        thread->setObjectName(QStringLiteral("sACNManagerPool %1").arg(i));
        thread->start(QThread::HighPriority);
        m_threadPool.push_back(thread);
    }
}

static void strongPointerDeleteListener(sACNListener * obj)
{
    obj->deleteLater();
    sACNManager::Instance().listenerDelete(obj);
}

static void strongPointerDeleteSender(sACNSentUniverse * obj)
{
    obj->deleteLater();
    sACNManager::Instance().senderDelete(obj);
}

sACNManager::tListener sACNManager::getListener(quint16 universe)
{
    QMutexLocker locker(&sACNManager_mutex);
    // Notes on the memory management of sACNListeners :
    // This function creates a QSharedPointer to the listener, which is handed to the classes that
    // want to use it. It stores a QWeakPointer, which gets set to null when all instances of the shared
    // pointer are gone

    QSharedPointer<sACNListener> strongPointer;
    if (!m_listenerHash.contains(universe))
    {
        qDebug() << "Creating listener for universe " << universe;

        // Create listener and move to thread
        sACNListener * listener = new sACNListener(universe);

        // Choose a thread
        QThread * thread = GetThread();
        listener->moveToThread(thread);

        connect(thread, &QThread::finished, listener, &sACNListener::deleteLater);
        connect(thread, &QThread::finished, thread, &sACNListener::deleteLater);

        // Emit sources from all, known, universes
        connect(
            listener,
            &sACNListener::sourceFound,
            this,
            [=](sACNSource * source) { emit sourceFound(universe, source); });
        connect(
            listener,
            &sACNListener::sourceLost,
            this,
            [=](sACNSource * source) { emit sourceLost(universe, source); });
        connect(
            listener,
            &sACNListener::sourceResumed,
            this,
            [=](sACNSource * source) { emit sourceResumed(universe, source); });
        connect(
            listener,
            &sACNListener::sourceChanged,
            this,
            [=](sACNSource * source) { emit sourceChanged(universe, source); });

        // Create strong pointer to return
        strongPointer = QSharedPointer<sACNListener>(listener, strongPointerDeleteListener);
        m_listenerHash[universe] = strongPointer.toWeakRef();
        m_objToUniverse[listener] = universe;

        // Start listening
        QMetaObject::invokeMethod(listener, "startReception", Qt::QueuedConnection);

        locker.unlock();
        emit newListener(universe);
    }
    else
    {
        strongPointer = m_listenerHash[universe].toStrongRef();
    }
    if (strongPointer.isNull())
    {
#ifdef QT_GUI_LIB
        QMessageBox msgBox;
        msgBox.setText(tr("Unable to allocate listener object\r\n\r\nsACNView must close now"));
        msgBox.exec();
#else
        qDebug() << "Unable to allocate listener object\r\n\r\nsACNView must close now";
#endif
        qApp->exit(-1);
    }

    return strongPointer;
}

void sACNManager::listenerDelete(QObject * obj)
{
    QMutexLocker locker(&sACNManager_mutex);
    int universe = m_objToUniverse[obj];

    qDebug() << "Destroying listener for universe " << universe;

    m_listenerHash.remove(universe);

    m_objToUniverse.remove(obj);

    emit deletedListener(universe);
}

QThread * sACNManager::GetThread()
{
    QThread * result = m_threadPool[m_nextThread];
    ++m_nextThread;
    if (m_nextThread == m_threadPool.size()) m_nextThread = 0;
    return result;
}

sACNManager::tSender sACNManager::createSender(CID cid, quint16 universe)
{
    qDebug() << "Creating sender for CID" << CID::CIDIntoQString(cid) << "universe" << universe;

    sACNSentUniverse * sender = new sACNSentUniverse(universe);
    sender->setCID(cid);
    connect(sender, &sACNSentUniverse::universeChange, this, &sACNManager::senderUniverseChanged);
    connect(sender, &sACNSentUniverse::cidChange, this, &sACNManager::senderCIDChanged);

    // Create strong pointer to return
    QSharedPointer<sACNSentUniverse> strongPointer = QSharedPointer<sACNSentUniverse>(
        sender,
        strongPointerDeleteSender);
    m_senderHash[cid][universe] = strongPointer.toWeakRef();
    m_objToUniverse[sender] = universe;
    m_objToCid[sender] = cid;

    sACNDiscoveryTX::getInstance()->sendDiscoveryPacketNow();

    return strongPointer;
}

sACNManager::tSender sACNManager::getSender(quint16 universe, CID cid)
{
    QMutexLocker locker(&sACNManager_mutex);
    // Notes on the memory management of sACNSenders :
    // This function creates a QSharedPointer to the sender, which is handed to the classes that
    // want to use it. It stores a QWeakPointer, which gets set to null when all instances of the shared
    // pointer are gone

    QSharedPointer<sACNSentUniverse> strongPointer;
    if (!m_senderHash.contains(cid))
    {
        strongPointer = createSender(cid, universe);
    }
    else
    {
        if (!m_senderHash[cid].contains(universe))
        {
            strongPointer = createSender(cid, universe);
        }
        else
        {
            strongPointer = m_senderHash[cid][universe].toStrongRef();
        }
    }

    if (strongPointer.isNull())
    {
#ifdef QT_GUI_LIB
        QMessageBox msgBox;
        msgBox.setText(tr("Unable to allocate sender object\r\n\r\nsACNView must close now"));
        msgBox.exec();
#else
        qDebug() << "Unable to allocate sender object\r\n\r\nsACNView must close now";
#endif
        qApp->exit(-1);
    }

    return strongPointer;
}

void sACNManager::senderDelete(QObject * obj)
{
    if (!m_objToUniverse.contains(obj) && !m_objToCid.contains(obj)) return;

    QMutexLocker locker(&sACNManager_mutex);
    quint16 universe = m_objToUniverse[obj];
    CID cid = m_objToCid[obj];

    qDebug() << "Destroying sender for CID" << CID::CIDIntoQString(cid) << "universe" << universe;

    m_senderHash[cid].remove(universe);

    m_objToUniverse.remove(obj);
    m_objToCid.remove(obj);
    emit deletedSender(cid, universe);
}

void sACNManager::senderUniverseChanged()
{
    if (!m_objToUniverse.contains(sender()) && !m_objToCid.contains(sender())) return;

    sACNSentUniverse * sACNSender = (sACNSentUniverse *)sender();
    if (!sACNSender) return;
    CID cid = m_objToCid[sender()];
    quint16 oldUniverse = m_objToUniverse[sender()];
    quint16 newUniverse = sACNSender->universe();

    m_senderHash[cid][newUniverse] = m_senderHash[cid][oldUniverse];
    m_senderHash[cid].remove(oldUniverse);

    m_objToUniverse[sender()] = newUniverse;

    qDebug()
        << "Sender for CID"
        << CID::CIDIntoQString(cid)
        << "was universe"
        << oldUniverse
        << "now universe"
        << newUniverse;
}

void sACNManager::senderCIDChanged()
{
    if (!m_objToUniverse.contains(sender()) && !m_objToCid.contains(sender())) return;

    sACNSentUniverse * sACNSender = (sACNSentUniverse *)sender();
    if (!sACNSender) return;
    CID oldCID = m_objToCid[sender()];
    CID newCID = sACNSender->cid();

    m_senderHash[newCID] = m_senderHash[oldCID];
    m_senderHash.remove(oldCID);

    m_objToCid[sender()] = newCID;

    qDebug() << "Sender CID" << CID::CIDIntoQString(oldCID) << "now CID" << CID::CIDIntoQString(newCID);
}
