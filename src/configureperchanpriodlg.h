#ifndef CONFIGUREPERCHANPRIODLG_H
#define CONFIGUREPERCHANPRIODLG_H

#include <QDialog>
#include "deftypes.h"
#include "consts.h"

namespace Ui {
class ConfigurePerChanPrioDlg;
}

class ConfigurePerChanPrioDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigurePerChanPrioDlg(QWidget *parent = 0);
    ~ConfigurePerChanPrioDlg();
    void setData(uint1 *data);
    uint1 *data();
public slots:
    void on_sbPriority_valueChanged(int value);
    void on_btnSetAll_pressed();
    void on_widget_selectedCellChanged(int cell);
private:
    Ui::ConfigurePerChanPrioDlg *ui;
    uint1 m_data[MAX_DMX_ADDRESS];
};

#endif // CONFIGUREPERCHANPRIODLG_H
