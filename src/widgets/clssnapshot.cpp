#include "clssnapshot.h"
#include <QPainter>
#include <QHBoxLayout>

clsSnapshot::clsSnapshot(quint16 universe, CID cid, QString name, QWidget *parent) : QWidget(parent),
    m_universe(universe),
    m_priority(DEFAULT_SACN_PRIORITY),
    m_cid(cid),
    m_sbUniverse(new QSpinBox(this)),
    m_sbPriority(new QSpinBox(this)),
    m_btnPlayback(new QToolButton(this)),
    m_lblStatus(new QLabel(this)),
    m_controlWidget(new QWidget(this)),
    m_sender(Q_NULLPTR),
    m_listener(Q_NULLPTR),
    m_camera(new QSound(":/sound/camera.wav", this)),
    m_backgroundMatches(false)
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
    updateIcons();

    m_sender = sACNManager::getInstance()->getSender(m_universe, m_cid);
    m_sender->setName(name);
    connect(m_sender.data(), &sACNSentUniverse::sendingTimeout, [this]() { emit senderTimedOut();} );
    setUniverse(m_universe);

    m_btnPlayback->setAutoRaise(true);
    m_controlWidget->setAutoFillBackground(true);
    QHBoxLayout *layout = new QHBoxLayout(m_controlWidget);
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addStretch();
    layout->addWidget(m_btnPlayback);
    layout->addWidget(m_lblStatus);
    layout->addStretch();
}

clsSnapshot::~clsSnapshot() {
}

void clsSnapshot::setUniverse(quint16 universe) {
    Q_ASSERT(universe >= MIN_SACN_UNIVERSE);
    Q_ASSERT(universe <= MAX_SACN_UNIVERSE);

    m_universe = universe;

    m_sbUniverse->setValue(m_universe);

    // Bug #197
    //if (m_listener)
    //    m_listener->deleteLater();
    m_listener = sACNManager::getInstance()->getListener(m_universe);
    connect(m_listener.data(), SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));
    connect(m_listener.data(), SIGNAL(sourceFound(sACNSource *source)), this, SLOT(levelsChanged()));
    connect(m_listener.data(), SIGNAL(sourceLost(sACNSource *source)), this, SLOT(levelsChanged()));
    connect(m_listener.data(), SIGNAL(sourceResumed(sACNSource *source)), this, SLOT(levelsChanged()));

    m_sender->setUniverse(m_universe);
}

void clsSnapshot::setPriority(quint8 priority) {
    Q_ASSERT(priority >= MIN_SACN_PRIORITY);
    Q_ASSERT(priority <= MAX_SACN_PRIORITY);

    m_priority = priority;

    m_sbPriority->setValue(m_priority);

    m_sender->setPerSourcePriority(m_priority);
}

void clsSnapshot::updateIcons() {
    if (!hasData())
        m_btnPlayback->setIcon(icons[ICON_SNAPSHOT]);
    else if (m_sender->isSending())
    {
        m_btnPlayback->setIcon(icons[ICON_PAUSE]);
    }
    else
        m_btnPlayback->setIcon(icons[ICON_PLAY]);

    if(!hasData())
    {
        m_lblStatus->setPixmap(statusIcons[STATUSICON_NONE]);
        m_lblStatus->setToolTip(statusIconTooltips[STATUSICON_NONE]);
    }
    else if(!m_sender->isSending())
    {
        m_lblStatus->setPixmap(statusIcons[STATUSICON_NONE]);
        m_lblStatus->setToolTip(statusIconTooltips[STATUSICON_NONE]);
    }
    else if(m_backgroundMatches)
    {
        m_lblStatus->setPixmap(statusIcons[STATUSICON_MATCHING]);
        m_lblStatus->setToolTip(statusIconTooltips[STATUSICON_MATCHING]);
    }
    else
    {
        m_lblStatus->setPixmap(statusIcons[STATUSICON_NOTMATCHING]);
        m_lblStatus->setToolTip(statusIconTooltips[STATUSICON_NOTMATCHING]);
    }

    // Enabled status
    m_sbUniverse->setEnabled(!isPlaying());
    m_sbPriority->setEnabled(!isPlaying());
}

void clsSnapshot::takeSnapshot() {
    m_levelData.clear();

    // Copy current merged universe
    for(int addr=0; addr<MAX_DMX_ADDRESS; addr++)
    {
        if (m_listener->mergedLevels().at(addr).level == -1)
            m_levelData.append(static_cast<char>(0));
        else
            m_levelData.append(m_listener->mergedLevels().at(addr).level);
    }

    updateIcons();

    emit snapshotTaken();
}

void clsSnapshot::playSnapshot() {
    if (!hasData())
    {
        updateIcons();
        return;
    }

    m_sender->startSending();
    m_sender->setLevel(reinterpret_cast<const quint8*>(m_levelData.constData()), std::min(m_levelData.count(), MAX_DMX_ADDRESS));
    updateIcons();
    emit senderStarted();
}

void clsSnapshot::stopSnapshot() {
    if (!hasData())
    {
        updateIcons();
        return;
    }

    m_sender->stopSending();
    updateIcons();
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

void clsSnapshot::levelsChanged()
{
    QByteArray background;

    int addr = 0;
    for (auto merged: m_listener->mergedLevels())
    {
        int level = 0;
        if ((merged.winningSource) && (merged.winningSource->src_cid == m_sender->cid()))
        {
            // I'm winning....
            if (merged.otherSources.isEmpty())
            {
                //...and the only source
                level = merged.level;
            } else {
                // ...find highest background priority
                int priority = 0;
                for (auto source: merged.otherSources)
                {
                    if (source->doing_per_channel)
                    {
                        if (source->priority_array[addr] > priority)
                        {
                            priority = source->priority;
                            level = source->level_array[addr];
                        }
                    } else {
                        if (source->priority > priority)
                        {
                            priority = source->priority;
                            level = source->level_array[addr];
                        }
                    }
                }
            }
        }
        else
        {
            // I'm not winning...
            level = merged.level;
        }

        background.append(level);

        if (addr < MAX_DMX_ADDRESS)
            addr++;
    }

    if ((background == m_levelData) != m_backgroundMatches)
    {
        m_backgroundMatches = (background == m_levelData);

        if (background == m_levelData)
            emit snapshotMatches();
        else
            emit snapshotDiffers();
    }

    updateIcons();
}

