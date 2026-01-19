#include "csvmodelexport.h"

#include <QFile>
#include <QLocale>
#include <QTextStream>

CsvModelExporter::CsvModelExporter(const QAbstractItemModel * model)
    : m_model(model)
{}

bool CsvModelExporter::saveAs(const QString & filename) const
{
    if (!m_model) return false;

    QFile csvFile(filename);
    if (!csvFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    QTextStream out(&csvFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif
    out.setLocale(QLocale::c());

    // Get column and row count for this tabular section
    const int columns = m_model->columnCount(m_rootIndex);

    // Output column headers
    for (int j = 0; j < columns; ++j)
    {
        QString header = m_model->headerData(j, Qt::Horizontal, Qt::DisplayRole).toString();

        out << formatField(header);

        if (j < (columns - 1)) out << m_sep;
    }

    out << m_recordSep;

    const int rows = m_model->rowCount(m_rootIndex);

    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < columns; ++j)
        {
            const QVariant field = m_model->data(m_model->index(i, j, m_rootIndex), Qt::DisplayRole);

            switch (field.typeId())
            {
                default:
                {
                    QString fieldString = field.toString();
                    out << formatField(fieldString);
                }
                break;
                case QMetaType::Bool: out << (field.toBool() ? QLatin1String("Yes") : QLatin1String("No")); break;
                case QMetaType::Float:
                case QMetaType::Double: out << field.toDouble(); break;
                case QMetaType::Int:
                case QMetaType::Long:
                case QMetaType::LongLong: out << field.toLongLong(); break;
                case QMetaType::UInt:
                case QMetaType::ULong:
                case QMetaType::ULongLong: out << field.toULongLong(); break;
            }

            if (j < (columns - 1)) out << m_sep;
        }
        out << m_recordSep;
    }

    return true;
}

QString & CsvModelExporter::formatField(QString & field) const
{
    // Escape quotes
    field.replace(m_quote, m_escapedQuote);

    // Enclose if necessary
    if (field.contains(m_sep) || field.contains(m_quote) || field.contains(m_recordSep))
    {
        field.prepend(m_quote);
        field.append(m_quote);
    }
    return field;
}
