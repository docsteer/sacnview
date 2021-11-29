#ifndef MONITORSPINBOX_H
#define MONITORSPINBOX_H

#include <QAbstractSpinBox>
#include "sacnlistener.h"

class monitorspinbox : public QAbstractSpinBox
{
    Q_OBJECT
public:
    monitorspinbox(QWidget* parent = nullptr);

    quint16 address() const { return m_address; }
    void setAddress(int universe, quint16 address);
    void setAddress(quint16 address) { setAddress(m_listener->universe(), address); }

    int universe() const { return m_listener->universe(); }

    quint8 level() const {
        auto level = m_listener->mergedLevels().at(m_address).level;
        return level > 0 ? level : 0;
    }

signals:
    void dataReady(int universe, quint16 address, QPointF data);

protected:
    void stepBy(int steps);
    QAbstractSpinBox::StepEnabled stepEnabled() const {return StepUpEnabled | StepDownEnabled; }

    QValidator::State validate(QString &input, int &pos) const;
    QValidator::State validate(const quint16 value) const;

    quint16 addressFromText(const QString &text) const;
    QString textFromAddress(quint16 address) const;

private:
    sACNManager::tListener m_listener;
    void setupListener(int universe);
    quint16 m_address;
};

#endif // MONITORSPINBOX_H
