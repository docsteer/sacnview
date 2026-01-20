#include <QAbstractTableModel>

#include "sacn/sacnsocket.h"
#include "sacn/streamingacn.h"

class SACNListenerTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    enum SC_COLS : int
    {
        COL_UNIVERSE,
        COL_MERGES_PER_SEC,
        COL_BIND_UNICAST,
        COL_BIND_MULTICAST,

        COL_COUNT
    };

public:

    SACNListenerTableModel(QObject * parent);
    ~SACNListenerTableModel();

    // Inherited via QAbstractTableModel
    Q_INVOKABLE int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    Q_INVOKABLE int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE QVariant data(const QModelIndex & index, int role) const override;

    Q_INVOKABLE void refresh();

private:

    struct UniverseDetails
    {
        sACNManager::wListener weakListener;
        int universe = 0;
    };
    QList<UniverseDetails> m_universeDetails;

    static QString bindStatus(sACNRxSocket::eBindStatus bindStatus);
};
