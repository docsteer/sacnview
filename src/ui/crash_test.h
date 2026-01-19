#ifndef CRASHTEST_H
#define CRASHTEST_H

#include <QDialog>
#include <QSignalMapper>

namespace Ui
{
    class CrashTest;
}

class CrashTest : public QDialog
{
    Q_OBJECT

public:

    explicit CrashTest(QWidget * parent = 0);
    ~CrashTest();

private slots:
    void crashMethod(const int id);

private:

    const uint numOfCrashMethods = 3;

    Ui::CrashTest * ui;
    QSignalMapper * m_signalMapper;
};

#endif // CRASHTEST_H
