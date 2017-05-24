#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QWidget>
#include "sacnlistener.h"

namespace Ui {
class LogWindow;
}

class LogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = 0);
    ~LogWindow();

    void setUniverse(int universe);
private slots:
    void onLevelsChanged();
    void on_btnCopyClipboard_pressed();
    void on_btnClear_pressed();
private:
    Ui::LogWindow *ui;

    QSharedPointer<sACNListener>m_listener;
};



#endif // LOGWINDOW_H
