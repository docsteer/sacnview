#include "monitorspinbox.h"
#include "consts.h"
#include "sacnlistener.h"
#include <QLineEdit>

monitorspinbox::monitorspinbox(QWidget* parent) : QAbstractSpinBox(parent),
    m_address(0)
{
    QAbstractSpinBox::setWrapping(true);

    connect(
        this->lineEdit(), &QLineEdit::textEdited,
        this, [this]() {
            QString input = lineEdit()->text();
            int pos = 0;
            if (QValidator::Acceptable == validate(input, pos))
                setAddress(addressFromText(input));
            else
                lineEdit()->setText(textFromAddress(this->address()));
        }
    );

    // Default listener
    setupListener(MIN_SACN_UNIVERSE);
}

void monitorspinbox::setupListener(int universe) {
    // New universe?
    if (m_listener == Q_NULLPTR || m_listener->universe() != universe) {
        m_listener = sACNManager::Instance().getListener(universe);

        disconnect(m_listener.data(), Q_NULLPTR, this, Q_NULLPTR);

        connect(
            m_listener.data(), &sACNListener::dataReady,
            this, [this, universe](int address, QPointF data) {
                if (m_listener->universe() == universe)
                    emit dataReady(universe, address, data);
            }
        );
    }
}

void monitorspinbox::setAddress(int universe, quint16 address)
{
    if (!validate(address))
        return;

    m_listener->unMonitorAddress(this->address(), this);

    m_address = address;
    setupListener(universe);
    m_listener->monitorAddress(this->address(), this);

    lineEdit()->setText(textFromAddress(this->address()));
}

void monitorspinbox::stepBy(int steps)
{
    quint16 newAddress = address() + steps;

    if (steps > 0)
        while (QValidator::Invalid == validate(newAddress)) ++newAddress;
    else if (steps < 0)
        while (QValidator::Invalid == validate(newAddress)) --newAddress;

    setAddress(newAddress);
}

QValidator::State monitorspinbox::validate(QString &input, int &pos) const {
    bool ok;
    quint16 value = QStringView{input}.mid(pos).toUShort(&ok);
    if (!ok)
        return QValidator::Invalid;

    return validate(value);
}

QValidator::State monitorspinbox::validate(const quint16 value) const {
    if (value > (MAX_DMX_ADDRESS - 1) || value < (MIN_DMX_ADDRESS - 1))
         return QValidator::Invalid;

    return QValidator::Acceptable;
}

quint16 monitorspinbox::addressFromText(const QString &text) const
{
    return text.toUShort() - 1;
}

QString monitorspinbox::textFromAddress(quint16 address) const
{
    return QString::number(address + 1);
}

