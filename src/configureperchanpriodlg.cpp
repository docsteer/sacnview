#include "configureperchanpriodlg.h"
#include "ui_configureperchanpriodlg.h"
#include "consts.h"

ConfigurePerChanPrioDlg::ConfigurePerChanPrioDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigurePerChanPrioDlg)
{
    ui->setupUi(this);
    ui->sbPriority->setMinimum(MIN_SACN_PRIORITY);
    ui->sbSetAll->setMinimum(MIN_SACN_PRIORITY);
    ui->sbPriority->setMaximum(MAX_SACN_PRIORITY);
    ui->sbSetAll->setMaximum(MAX_SACN_PRIORITY);
    ui->sbPriority->setValue(DEFAULT_SACN_PRIORITY);
    ui->sbSetAll->setValue(DEFAULT_SACN_PRIORITY);
}

ConfigurePerChanPrioDlg::~ConfigurePerChanPrioDlg()
{
    delete ui;
}


void ConfigurePerChanPrioDlg::on_sbPriority_valueChanged(int value)
{
    int currentCell = ui->widget->selectedCell();
    if(currentCell<0) return;
    ui->widget->setCellValue(currentCell, value);
    ui->widget->update();
}

void ConfigurePerChanPrioDlg::on_btnSetAll_pressed()
{
    for(int i=0; i<512; i++)
        ui->widget->setCellValue(i, ui->sbSetAll->value());
    ui->widget->update();
}

void ConfigurePerChanPrioDlg::on_widget_selectedCellChanged(int cell)
{
    if(cell>-1)
    {
        ui->sbPriority->setEnabled(true);
        ui->sbPriority->setValue(ui->widget->cellValue(cell));
        ui->lblAddress->setText(tr("Address %1, Priority = ").arg(cell+1));
    }
    else
    {
        ui->sbPriority->setEnabled(false);
        ui->lblAddress->setText("");
    }
}
