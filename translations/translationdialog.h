#ifndef TRANSLATIONDIALOG_H
#define TRANSLATIONDIALOG_H

#include <QDialog>
#include <QTranslator>
#include <QVBoxLayout>
#include <QSignalMapper>

namespace Ui {
class TranslationDialog;
}

class TranslationDialog : public QDialog
{
    Q_OBJECT

public:
    /* Translation dialog/handler
     *
     * @arg CurrentFilename (Optional) - Fullpath to current tranlation file
     * @arg VBoxLayout (Optional) - VBoxLayout to fill with checkboxes, if nullptr a dialog is created and displayed
     */
    explicit TranslationDialog(const QString CurrentFilename = QString(""), QVBoxLayout *VBoxLayout = Q_NULLPTR, QWidget *parent = 0);
    ~TranslationDialog();

    /* exec()
     *
     * Display dialog, if multiple options avaliabe, and block until close
     */
    virtual int exec();

public slots:
    void LoadTranslation(const QString filename);

private slots:
    void on_pbOk_clicked();

private:
    Ui::TranslationDialog *ui;
    QSignalMapper* m_signalMapper;
};

#endif // TRANSLATIONDIALOG_H
