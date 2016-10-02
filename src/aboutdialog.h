#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

namespace Ui {
class aboutDialog;
}

class aboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit aboutDialog(QWidget *parent = 0);
    ~aboutDialog();

private slots:
    void updateDisplay();

private:
    Ui::aboutDialog *ui;
    QTimer *m_displayTimer;

};

#endif // ABOUTDIALOG_H