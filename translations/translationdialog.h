#ifndef TRANSLATIONDIALOG_H
#define TRANSLATIONDIALOG_H

#include <QDialog>
#include <QTranslator>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QLocale>


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
    explicit TranslationDialog(const QLocale DefaultLocale, QVBoxLayout *VBoxLayout = Q_NULLPTR, QWidget *parent = Q_NULLPTR);
    ~TranslationDialog();

    /* exec()
     *
     * Display dialog, if multiple options avaliabe, and block until close
     */
    virtual int exec();

public:
    bool LoadTranslation(const QLocale locale);
    bool LoadTranslation() {
        return LoadTranslation(m_locate);
    }

    QLocale GetSelectedLocale();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::TranslationDialog *ui;
    QButtonGroup *m_bgTranslations;
    QLocale m_locate;
};

#endif // TRANSLATIONDIALOG_H
