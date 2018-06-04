#include "translationdialog.h"
#include "ui_translationdialog.h"
#include "translations.h"
#include "preferences.h"

#include <QRadioButton>
#include <QDir>
#include <QButtonGroup>
#include <QDebug>

TranslationDialog::TranslationDialog(const QString CurrentFilename, QVBoxLayout *VBoxLayout, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TranslationDialog),
    m_signalMapper(new QSignalMapper(this))
{
    ui->setupUi(this);
    if (VBoxLayout == Q_NULLPTR)
        VBoxLayout = ui->vlSelectLang;

    // Translations
    auto *bgTranslations = new QButtonGroup(this);
    bgTranslations->setExclusive(true);
    for (auto translation : Translations::lTranslations)
    {
        if (QFile::exists(translation.FileName)) {
            qDebug() << "[Translate] Found" << translation.FileName;
            QRadioButton *rb = new QRadioButton(this);
            bgTranslations->addButton(rb);
            rb->setText(translation.LanguageName);
            rb->setProperty("Filename", translation.FileName);
            // Check current translation, or default
            if (!CurrentFilename.isEmpty())
                rb->setChecked(CurrentFilename.contains(translation.FileName));
            else if (bgTranslations->checkedId() == -1)
                rb->setChecked(true);
            connect(rb, SIGNAL(pressed()), m_signalMapper, SLOT(map()));
            m_signalMapper->setMapping(rb, translation.FileName);
            VBoxLayout->addWidget(rb);
        } else {
            qDebug() << "[Translate] Not Found" << translation.FileName;
        }
    }

    connect(m_signalMapper, SIGNAL(mapped(QString)),
            this, SLOT(LoadTranslation(QString)));
}

TranslationDialog::~TranslationDialog()
{
    delete ui;
}

int TranslationDialog::exec()
{
    // Only show dialog if there are options to select!
    if (ui->vlSelectLang->count() > 2)
        QDialog::exec();

    return 0;
}

void TranslationDialog::on_pbOk_clicked()
{
    this->close();
}

void TranslationDialog::LoadTranslation(const QString filename)
{
    if (Preferences::getInstance()->CurrentTranslator)
        qApp->removeTranslator(Preferences::getInstance()->CurrentTranslator);
    else
        Preferences::getInstance()->CurrentTranslator = new QTranslator();

    qDebug() << "[Translate] Selected language file" << filename;
    Preferences::getInstance()->SetTranslationFilename(filename);
    if (QFile::exists(filename)) {
        qDebug() << "[Translate] Loading" << filename;
        Preferences::getInstance()->CurrentTranslator->load(filename);
        qApp->installTranslator(Preferences::getInstance()->CurrentTranslator);
    } else {
        qDebug() << "[Translate] Not found" << filename;
    }
}
