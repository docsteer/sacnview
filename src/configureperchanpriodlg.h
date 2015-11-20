// Copyright (c) 2015 Tom Barthel-Steer, http://www.tomsteer.net
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


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
