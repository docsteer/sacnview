#include "bigdisplay.h"
#include "ui_bigdisplay.h"
#include "preferences.h"

BigDisplay::BigDisplay(int universe, quint16 address, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BigDisplay)
{
    ui->setupUi(this);

    // 8bit controls
    ui->spinBox_8->setAddress(universe, address);
    connect(ui->spinBox_8, &monitorspinbox::dataReady, this, &BigDisplay::dataReady);

    // 16bit controls
    ui->spinBox_16_Coarse->setAddress(universe, address);
    connect(ui->spinBox_16_Coarse, &monitorspinbox::dataReady, this, &BigDisplay::dataReady);
    //--
    ui->spinBox_16_Fine->setAddress(universe, address + 1);
    connect(ui->spinBox_16_Fine, &monitorspinbox::dataReady, this, &BigDisplay::dataReady);

    // Colour controls
    ui->spinBox_RGB_1->setAddress(universe, address);
    connect(ui->spinBox_RGB_1, &monitorspinbox::dataReady, this, &BigDisplay::dataReady);
    //--
    ui->spinBox_RGB_2->setAddress(universe, address + 1);
    connect(ui->spinBox_RGB_2, &monitorspinbox::dataReady, this, &BigDisplay::dataReady);
    //--
    ui->spinBox_RGB_3->setAddress(universe, address + 2);
    connect(ui->spinBox_RGB_3, &monitorspinbox::dataReady, this, &BigDisplay::dataReady);

    // Window
    this->setWindowTitle(tr("Big Display Universe %1").arg(universe));
    this->setWindowIcon(parent->windowIcon());

    m_level = 0;
    displayLevel();
}

BigDisplay::~BigDisplay()
{
    delete ui;
}

void BigDisplay::displayLevel()
{
    QPalette palette = ui->lcdNumber->palette();
    QColor colour(m_level | 0xFF000000);

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
                ui->lcdNumber->display((int)m_level);
                break;
            }
            case Preferences::HEXADECIMAL:
            {
                ui->lcdNumber->setMode(QLCDNumber::Hex);
                ui->lcdNumber->display((int)m_level);
                break;
            }
            case Preferences::PERCENT:
            {
                ui->lcdNumber->setMode(QLCDNumber::Dec);
                // Display percent with an optional fraction
                if(ui->tabWidget->currentIndex() == tabModes_bit8)
                {
                    ui->lcdNumber->display(HTOPT[m_level & 0xFF]);
                }
                else
                {
                    int percent = HTOPT[(m_level & 0xFF00) >> 8];
                    int fraction = m_level & 0xFF;
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
        ui->lcdNumber->display((int)m_level);
    }
}

void BigDisplay::dataReady(int universe, quint16 address, QPointF data)
{
    quint8 level = data.y() > 0 ? data.y() : 0;

    switch (ui->tabWidget->currentIndex())
    {
        case tabModes_bit8:
            if (address == ui->spinBox_8->address() && universe == ui->spinBox_8->universe())
                m_level = level;

            break;

        case tabModes_bit16:
            if (address == ui->spinBox_16_Coarse->address() && universe == ui->spinBox_16_Coarse->universe())
                m_level = (m_level & 0x00FF) | (0xFF00 & (level << 8));

            if (address == ui->spinBox_16_Fine->address() && universe == ui->spinBox_16_Fine->universe())
                m_level = (m_level & 0xFF00) | (0x00FF & level);

            break;

        case tabModes_rgb:
            if (address == ui->spinBox_RGB_1->address() && universe == ui->spinBox_RGB_1->universe())
                m_level = (m_level & 0x00FFFFu) | (0xFF0000u & ((quint32)level << 16));

            if (address == ui->spinBox_RGB_2->address() && universe == ui->spinBox_RGB_2->universe())
                m_level = (m_level & 0xFF00FFu) | (0x00FF00u & ((quint32)level << 8));

            if (address == ui->spinBox_RGB_3->address() && universe == ui->spinBox_RGB_3->universe())
                m_level = (m_level & 0xFFFF00u) | (0x0000FFu & level);

            break;

        default:
            m_level = -1;
            break;
    }

    displayLevel();
}

void BigDisplay::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index);
    m_level = 0;
    displayLevel();
}
