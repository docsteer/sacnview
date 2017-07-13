#include "bigdisplay.h"
#include "ui_bigdisplay.h"

BigDisplay::BigDisplay(int universe, quint16 address, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BigDisplay),
    m_universe(universe)
{
    ui->setupUi(this);

    // Make address human readable
    address++;

    // 8bit controls
    ui->spinBox_8->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_8->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_8->setValue(address);
    ui->spinBox_8->setWrapping(true);

    // 16bit controls
    ui->spinBox_16_Coarse->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_16_Coarse->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_16_Coarse->setValue(address);
    ui->spinBox_16_Coarse->setWrapping(true);
    //--
    ui->spinBox_16_Fine->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_16_Fine->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_16_Fine->setValue(address + 1);
    ui->spinBox_16_Fine->setWrapping(true);

    // Colour controls
    ui->spinBox_RGB_1->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_RGB_1->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_RGB_1->setValue(address);
    ui->spinBox_RGB_1->setWrapping(true);
    //--
    ui->spinBox_RGB_2->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_RGB_2->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_RGB_2->setValue(address + 1);
    ui->spinBox_RGB_2->setWrapping(true);
    //--
    ui->spinBox_RGB_3->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_RGB_3->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_RGB_3->setValue(address + 2);
    ui->spinBox_RGB_3->setWrapping(true);

    // Window
    this->setWindowTitle(tr("Big Display Universe %1").arg(m_universe));
    this->setWindowIcon(parent->windowIcon());

    // Listen to universe
    m_listener = sACNManager::getInstance()->getListener(m_universe);
    connect(m_listener.data(), SIGNAL(dataReady(int, QPointF)), this, SLOT(dataReady(int, QPointF)));

    setupAddressMonitors();
}

BigDisplay::~BigDisplay()
{
    delete ui;
}

void BigDisplay::setupAddressMonitors()
{
    m_listener->monitorAddress(ui->spinBox_8->value() - 1);

    m_listener->monitorAddress(ui->spinBox_16_Coarse->value() - 1);
    m_listener->monitorAddress(ui->spinBox_16_Fine->value() - 1);

    m_listener->monitorAddress(ui->spinBox_RGB_1->value() - 1);
    m_listener->monitorAddress(ui->spinBox_RGB_2->value() - 1);
    m_listener->monitorAddress(ui->spinBox_RGB_3->value() - 1);
}

void BigDisplay::dataReady(int address, QPointF data)
{
    bool required = false;
    int valueMax = 0;
    int value = -1;
    address++;

    QPalette palette = ui->lcdNumber->palette();
    QColor colour = palette.windowText().color();


    switch (ui->tabWidget->currentIndex())
    {
        case tabModes_bit8:
        {
            valueMax = 255;
            colour.setRgb(Qt::black);

            if (address == ui->spinBox_8->value()) {
                required = true;
                value = data.y();
            }
            break;
        }

        case tabModes_bit16:
        {
            valueMax = 65535;
            colour.setRgb(Qt::black);

            if (address == ui->spinBox_16_Coarse->value()) {
                required = true;
                value = ui->lcdNumber->intValue() & 0x00FF;
                value += int(data.y()) << 8;
            }

            if (address == ui->spinBox_16_Fine->value()) {
                required = true;
                value = (ui->lcdNumber->intValue() & 0xFF00);
                value += int(data.y());
            }

            break;
        }

        case tabModes_rgb:
        {
            if (address == ui->spinBox_RGB_1->value()) {
                required = true;
                colour.setRed(int(data.y()));
            }

            if (address == ui->spinBox_RGB_2->value()) {
                required = true;
                colour.setGreen(int(data.y()));
            }

            if (address == ui->spinBox_RGB_3->value()) {
                required = true;
                colour.setBlue(int(data.y()));

            }

            value = (colour.red() + colour.green() + colour.blue()) / 3;
            break;
        }

        default:
        {
            value = 0;
            break;
        };
    }

    // Unrequired address?
    if (!required)
    {
        m_listener->unMonitorAddress(address);
        setupAddressMonitors();
    }

    if (value < 0)
    {
        return;
    }

    // Display format
    switch (Preferences::getInstance()->GetDisplayFormat())
    {
        default:
        case Preferences::DECIMAL:
        {
            ui->lcdNumber->setMode(QLCDNumber::Dec);
            break;
        }
        case Preferences::HEXADECIMAL:
        {
            ui->lcdNumber->setMode(QLCDNumber::Hex);
            break;
        }
        case Preferences::PERCENT:
        {
            ui->lcdNumber->setMode(QLCDNumber::Dec);
            value = (value * 100) / valueMax;
            break;
        }
    }

    palette.setColor(QPalette::WindowText, colour);
    ui->lcdNumber->setPalette(palette);

    ui->lcdNumber->display(value);
}

void BigDisplay::on_spinBox_8_editingFinished()
{
    setupAddressMonitors();
}

void BigDisplay::on_spinBox_16_Coarse_editingFinished()
{
    setupAddressMonitors();
}

void BigDisplay::on_spinBox_16_Fine_editingFinished()
{
    setupAddressMonitors();
}

void BigDisplay::on_spinBox_Colour_1_editingFinished()
{
    setupAddressMonitors();
}

void BigDisplay::on_spinBox_Colour_2_editingFinished()
{
    setupAddressMonitors();
}

void BigDisplay::on_spinBox_Colour_3_editingFinished()
{
    setupAddressMonitors();
}
