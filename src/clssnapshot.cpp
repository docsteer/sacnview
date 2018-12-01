#include "clssnapshot.h"

clsSnapshot::clsSnapshot(quint16 universe, CID cid, QString name, QWidget *parent) : QWidget(parent),
    m_universe(universe),
    m_priority(DEFAULT_SACN_PRIORITY),
    m_cid(cid),
    m_sbUniverse(new QSpinBox(this)),
    m_sbPriority(new QSpinBox(this)),
    m_btnPlayback(new QToolButton(this)),
    m_sender(Q_NULLPTR),
    m_listener(Q_NULLPTR),
    m_camera(new QSound(":/sound/camera.wav", this))
{
    m_sbUniverse->setMinimum(MIN_SACN_UNIVERSE);
    m_sbUniverse->setMaximum(MAX_SACN_UNIVERSE);
    m_sbUniverse->setValue(m_universe);
    connect(m_sbUniverse, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, [this](int value) { setUniverse(value); } );

    m_sbPriority->setMinimum(MIN_SACN_PRIORITY);
    m_sbPriority->setMaximum(MAX_SACN_PRIORITY);
    m_sbPriority->setValue(m_priority);
    connect(m_sbPriority, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, [this](int value) { setPriority(value); } );

    connect(m_btnPlayback, SIGNAL(clicked(bool)), this, SLOT(btnEnableClicked(bool)));
    m_btnPlayback->setIcon(icons[ICON_SNAPSHOT]);

    m_sender = sACNManager::getInstance()->getSender(m_universe, m_cid);
    m_sender->setName(name);
    connect(m_sender.data(), &sACNSentUniverse::sendingTimeout, [this]() { emit senderTimedOut();} );

    m_listener = sACNManager::getInstance()->getListener(m_universe);
}

clsSnapshot::~clsSnapshot() {
}

void clsSnapshot::setUniverse(quint16 universe) {
    Q_ASSERT(universe >= MIN_SACN_UNIVERSE);
    Q_ASSERT(universe <= MAX_SACN_UNIVERSE);

    m_universe = universe;

    m_sbUniverse->setValue(m_universe);

    m_listener->deleteLater();
    m_listener = sACNManager::getInstance()->getListener(m_universe);

    m_sender->setUniverse(m_universe);
}

void clsSnapshot::setPriority(quint8 priority) {
    Q_ASSERT(priority >= MIN_SACN_PRIORITY);
    Q_ASSERT(priority <= MAX_SACN_PRIORITY);

    m_priority = priority;

    m_sbPriority->setValue(m_priority);

    m_sender->setPerSourcePriority(m_priority);
}

void clsSnapshot::takeSnapshot() {
    m_levelData.clear();

    // Copy current merged universe
    for(int addr=0; addr<MAX_DMX_ADDRESS; addr++)
    {
        m_levelData.append(m_listener->mergedLevels().at(addr).level);
    }

    m_btnPlayback->setIcon(icons[ICON_PLAY]);

    emit snapshotTaken();
}

void clsSnapshot::playSnapshot() {
    if (!hasData())
    {
        m_btnPlayback->setIcon(icons[ICON_SNAPSHOT]);
        return;
    }

    m_sender->startSending();
    m_sender->setLevel(reinterpret_cast<const quint8*>(m_levelData.constData()), std::min(m_levelData.count(), MAX_DMX_ADDRESS));
    m_btnPlayback->setIcon(icons[ICON_PAUSE]);
    emit senderStarted();
}

void clsSnapshot::stopSnapshot() {
    if (!hasData())
    {
        m_btnPlayback->setIcon(icons[ICON_SNAPSHOT]);
        return;
    }

    m_sender->stopSending();
    m_btnPlayback->setIcon(icons[ICON_PLAY]);
    emit senderStopped();
}

void clsSnapshot::btnEnableClicked(bool value) {
    Q_UNUSED(value);
    if (!hasData())
    {
        m_camera->play();
        takeSnapshot();
        return;
    } else if (isPlaying())
        stopSnapshot();
    else
        playSnapshot();
}
