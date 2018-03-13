#ifndef XPWARNING_H
#define XPWARNING_H

#include <QMessageBox>

/*
 *  Returns true if Windows XP
 */
bool XPOnlyFeature() {
    #ifdef TARGET_WINXP)
        QMessageBox msgBox;
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(QObject::tr("This binary is intended for Windows XP only\r\nThis feature is unavailable"));
        msgBox.exec();
        return true;
    #else
        return false;
    #endif
}

#endif // XPWARNING_H
