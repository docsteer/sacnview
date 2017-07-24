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

    m_data = 0;

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

void BigDisplay::displayData()
{
    QPalette palette = ui->lcdNumber->palette();
    QColor colour(m_data | 0xFF000000);

    // Display format
    if((ui->tabWidget->currentIndex() == tabModes_bit8) |
          (ui->tabWidget->currentIndex() == tabModes_bit16)  )
    {
        ui->lcdNumber->setDigitCount(5);
        palette.setColor(QPalette::WindowText, Qt::red);
        ui->lcdNumber->setPalette(palette);

        switch (Preferences::getInstance()->GetDisplayFormat())
        {
            default:
            case Preferences::DECIMAL:
            {
                ui->lcdNumber->setMode(QLCDNumber::Dec);
                ui->lcdNumber->display((int)m_data);
                break;
            }
            case Preferences::HEXADECIMAL:
            {
                ui->lcdNumber->setMode(QLCDNumber::Hex);
                ui->lcdNumber->display((int)m_data);
                break;
            }
            case Preferences::PERCENT:
            {
                ui->lcdNumber->setMode(QLCDNumber::Dec);
                // Display percent with an optional fraction
                if(ui->tabWidget->currentIndex() == tabModes_bit8)
                {
                    ui->lcdNumber->display(HTOPT[m_data & 0xFF]);
                }
                else
                {
                    int percent = HTOPT[(m_data & 0xFF00) >> 8];
                    int fraction = m_data & 0xFF;
                    double value = percent + fraction/255.0;
                    ui->lcdNumber->display(value);
                }
                break;
            }
        }
    }

    if(ui->tabWidget->currentIndex() == tabModes_rgb)
    {
        // Only ever display RGB as hex

        palette.setColor(QPalette::WindowText, colour);
        ui->lcdNumber->setPalette(palette);
        ui->lcdNumber->setDigitCount(6);
        ui->lcdNumber->setMode(QLCDNumber::Hex);
        ui->lcdNumber->display((int)m_data);
    }



}

void BigDisplay::dataReady(int address, QPointF data)
{
    bool required = false;
    address++;
    quint32 value = data.y();


    switch (ui->tabWidget->currentIndex())
    {
        case tabModes_bit8:
            if (address == ui->spinBox_8->value()) {
                required = true;
                m_data = data.y();
            }
            break;

        case tabModes_bit16:
            if (address == ui->spinBox_16_Coarse->value()) {
                required = true;
                m_data = (m_data & 0x00FF) | (0xFF00 & (value << 8));
            }

            if (address == ui->spinBox_16_Fine->value()) {
                required = true;
                m_data = (m_data & 0xFF00) | (0x00FF & value);
            }

            break;

        case tabModes_rgb:
            if (address == ui->spinBox_RGB_1->value()) {
                required = true;
                m_data = (m_data & 0x00FFFFu) | (0xFF0000u & ((quint32)value << 16));
            }

            if (address == ui->spinBox_RGB_2->value()) {
                required = true;
                m_data = (m_data & 0xFF00FFu) | (0x00FF00u & ((quint32)value << 8));
            }

            if (address == ui->spinBox_RGB_3->value()) {
                required = true;
                m_data = (m_data & 0xFFFF00u) | (0x0000FFu & value);
            }
        break;

        default:
            m_data = -1;
            break;
    }

    // Unrequired address?
    if (!required)
    {
        m_listener->unMonitorAddress(address);
        setupAddressMonitors();
    }

    displayData();
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

void BigDisplay::on_spinBox_RGB_1_editingFinished()
{
    setupAddressMonitors();
}

void BigDisplay::on_spinBox_RGB_2_editingFinished()
{
    setupAddressMonitors();
}

void BigDisplay::on_spinBox_RGB_3_editingFinished()
{
    setupAddressMonitors();
}
