#include "crash_test.h"
#include "ui_crash_test.h"

#include <QPushButton>

CrashTest::CrashTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CrashTest)
{
    ui->setupUi(this);

    m_signalMapper = new QSignalMapper(this);

    for (uint n = 0; n < numOfCrashMethods; n++) {
        QPushButton *b = new QPushButton(this);
        b->setText(QString("Method %1").arg(n));
        connect(b, &QPushButton::clicked, m_signalMapper, QOverload<>::of(&QSignalMapper::map));
        m_signalMapper->setMapping(b, n);

        ui->verticalLayout->addWidget(b);
    }

    connect(m_signalMapper, &QSignalMapper::mappedInt, this, &CrashTest::crashMethod);
}

CrashTest::~CrashTest()
{
    delete ui;
}

void CrashTest::crashMethod(const int id)
{
    switch (id) {
        default:
        case 0:
        {
            abort();
            break;
        }
        case 1:
        {
            void* jump = nullptr;
            ((void(*)())jump)();
            break;
        }
        case 2:
        {
            *((int*) 0) = 0;
        }
    }

}
