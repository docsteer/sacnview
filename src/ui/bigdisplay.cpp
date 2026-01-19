#include "bigdisplay.h"
#include "preferences.h"
#include "ui_bigdisplay.h"

#include "sacn/sacnlistener.h"

BigDisplay::BigDisplay(int universe, quint16 slot_index, QWidget * parent)
    : QWidget(parent), ui(new Ui::BigDisplay), m_listener(sACNManager::Instance().getListener(universe))
{
    ui->setupUi(this);

    int address = slot_index + 1;

    // 8bit controls
    ui->spinBox_8->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_8->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_8->setValue(address);

    // 16bit controls
    ui->spinBox_16_Coarse->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_16_Coarse->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_16_Fine->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_16_Fine->setMaximum(MAX_DMX_ADDRESS);

    ui->spinBox_16_Coarse->setValue(address);
    ui->spinBox_16_Fine->setValue(address + 1);

    // Colour controls
    ui->spinBox_RGB_1->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_RGB_1->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_RGB_2->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_RGB_2->setMaximum(MAX_DMX_ADDRESS);
    ui->spinBox_RGB_3->setMinimum(MIN_DMX_ADDRESS);
    ui->spinBox_RGB_3->setMaximum(MAX_DMX_ADDRESS);

    ui->spinBox_RGB_1->setValue(address);
    ui->spinBox_RGB_2->setValue(address + 1);
    ui->spinBox_RGB_3->setValue(address + 2);

    // Window
    this->setWindowTitle(tr("Big Display Universe %1").arg(universe));
    this->setWindowIcon(parent->windowIcon());

    updateLevels();

    // Refresh every four DMX frames or so
    startTimer(88);
}

BigDisplay::~BigDisplay()
{
    delete ui;
}

void BigDisplay::displayLevel()
{
    QPalette palette = ui->lcdNumber->palette();

    // Display format
    if ((ui->tabWidget->currentIndex() == tabModes_bit8) || (ui->tabWidget->currentIndex() == tabModes_bit16))
    {
        ui->lcdNumber->setDigitCount(5);
        // Grey when not active
        palette.setColor(QPalette::WindowText, m_active ? Qt::red : Qt::gray);
        ui->lcdNumber->setPalette(palette);

        switch (Preferences::Instance().GetDisplayFormat())
        {
            default:
            case DisplayFormat::DECIMAL:
            {
                ui->lcdNumber->setMode(QLCDNumber::Dec);
                ui->lcdNumber->display(m_level);
                break;
            }
            case DisplayFormat::HEXADECIMAL:
            {
                ui->lcdNumber->setMode(QLCDNumber::Hex);
                ui->lcdNumber->display(m_level);
                break;
            }
            case DisplayFormat::PERCENT:
            {
                ui->lcdNumber->setMode(QLCDNumber::Dec);
                // Display percent with an optional fraction
                if (ui->tabWidget->currentIndex() == tabModes_bit8)
                {
                    ui->lcdNumber->display(HTOPT[m_level & 0xFF]);
                }
                else
                {
                    int percent = HTOPT[(m_level & 0xFF00) >> 8];
                    int fraction = m_level & 0xFF;
                    double value = percent + fraction / 255.0;
                    ui->lcdNumber->display(value);
                }
                break;
            }
        }
    }

    if (ui->tabWidget->currentIndex() == tabModes_rgb)
    {
        // Only ever display RGB as hex
        QColor colour(static_cast<uint32_t>(m_level) | 0xFF000000);
        palette.setColor(QPalette::WindowText, colour);
        ui->lcdNumber->setPalette(palette);
        ui->lcdNumber->setDigitCount(6);
        ui->lcdNumber->setMode(QLCDNumber::Hex);
        ui->lcdNumber->display(m_level);
    }
}

void BigDisplay::timerEvent(QTimerEvent * /*ev*/)
{
    updateLevels();
}

void BigDisplay::on_tabWidget_currentChanged(int /*index*/)
{
    updateLevels();
}

void BigDisplay::updateLevels()
{
    // Get and calculate levels
    const auto levels = m_listener->mergedLevelsOnly();

    int level = -1;

    switch (ui->tabWidget->currentIndex())
    {
        default: break;

        case tabModes_bit8:
        {
            level = levels[ui->spinBox_8->value() - 1];
        }
        break;

        case tabModes_bit16:
        {
            const int coarse = levels[ui->spinBox_16_Coarse->value() - 1];
            const int fine = levels[ui->spinBox_16_Fine->value() - 1];
            if (coarse >= 0 && fine >= 0)
            {
                level = static_cast<uint32_t>(coarse) << 8 | static_cast<uint32_t>(fine);
            }
        }
        break;

        case tabModes_rgb:
        {
            const int red = levels[ui->spinBox_RGB_1->value() - 1];
            const int green = levels[ui->spinBox_RGB_2->value() - 1];
            const int blue = levels[ui->spinBox_RGB_3->value() - 1];
            if (red >= 0 && green >= 0 && blue >= 0)
            {
                level = static_cast<uint32_t>(red) << 16
                    | static_cast<uint32_t>(green) << 8
                    | static_cast<uint32_t>(blue);
            }
        }
        break;
    }

    if (level < 0)
    {
        if (!m_active) return;
        m_active = false;
    }
    else
    {
        if (m_active && m_level == level) return;

        m_level = level;
        m_active = true;
    }

    displayLevel();
}
