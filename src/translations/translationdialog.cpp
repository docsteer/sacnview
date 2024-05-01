#include "translationdialog.h"
#include "ui_translationdialog.h"
#include "translations.h"
#include "preferences.h"

#include <QRadioButton>
#include <QDir>
#include <QButtonGroup>
#include <QLibraryInfo>
#include <QDebug>

#define TRANSLATION_QM_DIR ":/i18n/"

TranslationDialog::TranslationDialog(const QLocale DefaultLocale, QVBoxLayout *VBoxLayout, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TranslationDialog),
    m_bgTranslations(new QButtonGroup(this)),
    m_locate(DefaultLocale)
{
    ui->setupUi(this);
    if (VBoxLayout == Q_NULLPTR)
        VBoxLayout = ui->vlSelectLang;

    // Translations
    m_bgTranslations->setExclusive(true);
    for (const auto &translation : Translations::lTranslations)
    {
        QRadioButton *rb = new QRadioButton(this);
        m_bgTranslations->addButton(rb);
        rb->setText(translation.NativeLanguageName);
        rb->setProperty("Language", translation.Language);
        rb->setChecked((translation.Language == DefaultLocale.language()));
        VBoxLayout->addWidget(rb);
    }
}

TranslationDialog::~TranslationDialog()
{
    delete ui;
}

int TranslationDialog::exec()
{
    // Only show dialog if there are options to select!
    if (ui->vlSelectLang->count())
        QDialog::exec();

    return 0;
}

bool TranslationDialog::LoadTranslation(const QLocale locale)
{
    // Default
    if (locale.language() == QLocale::English)
        return true;

    // Exists in resource
    bool ret = false;
    for (const auto &lang : locale.uiLanguages()) {
        ret = QFile(QString("%1%2_%3.qm").arg(TRANSLATION_QM_DIR, qApp->applicationName(), lang)).exists();
        if (ret) break;
    }
    if (ret) {
        qDebug() << "[Translate] Loading " << locale.uiLanguages();

        // Load application translation from resource
        {
            QTranslator *translator = new QTranslator();
            if (translator->load(locale, qApp->applicationName(), "_", TRANSLATION_QM_DIR, ".qm"))
                qApp->installTranslator(translator);
        }

        // Load QT translations
        {
            QTranslator *translator = new QTranslator();
            if (translator->load(locale, "qt", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
                qApp->installTranslator(translator);
        }
        {
            QTranslator *translator = new QTranslator();
            if (translator->load(locale, "qtbase", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
                qApp->installTranslator(translator);
        }

    } else {
        qDebug() << "[Translate] Not found requested " << locale.uiLanguages();
    }

    return ret;
}

QLocale TranslationDialog::GetSelectedLocale()
{
    if (m_bgTranslations->checkedButton()) {
        return QLocale(static_cast<QLocale::Language>(m_bgTranslations->checkedButton()->property("Language").toInt()));
    } else {
        return QLocale(QLocale::AnyLanguage);
    }
}

void TranslationDialog::on_buttonBox_accepted()
{
    qDebug() << "[Translate] User selected " << GetSelectedLocale().uiLanguages();
    Preferences::Instance().SetLocale(GetSelectedLocale());

    this->close();
}
