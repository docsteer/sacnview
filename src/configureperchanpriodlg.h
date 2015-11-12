#ifndef CONFIGUREPERCHANPRIODLG_H
#define CONFIGUREPERCHANPRIODLG_H

#include <QDialog>

namespace Ui {
class ConfigurePerChanPrioDlg;
}

class ConfigurePerChanPrioDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigurePerChanPrioDlg(QWidget *parent = 0);
    ~ConfigurePerChanPrioDlg();
public slots:
    void on_sbPriority_valueChanged(int value);
    void on_btnSetAll_pressed();
    void on_widget_selectedCellChanged(int cell);
private:
    Ui::ConfigurePerChanPrioDlg *ui;
};

#endif // CONFIGUREPERCHANPRIODLG_H
