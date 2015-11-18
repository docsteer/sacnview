#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "consts.h"

aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutDialog)
{
    ui->setupUi(this);

    ui->DisplayVer->setText(VERSION);
    ui->displayDate->setText(PUBLISHED_DATE);
    ui->DisplayName->setText(AUTHOR);

}

aboutDialog::~aboutDialog()
{
    delete ui;
}
