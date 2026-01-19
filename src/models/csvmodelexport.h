// Copyright 2023 Richard Thompson
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <QAbstractItemModel>

class CsvModelExporter
{
public:

    explicit CsvModelExporter() {}
    explicit CsvModelExporter(const QAbstractItemModel * model);

    void setModel(const QAbstractItemModel * model) { m_model = model; }
    void setRootIndex(const QModelIndex & index) { m_rootIndex = index; }

    bool saveAs(const QString & filename) const;

private:

    const QAbstractItemModel * m_model = nullptr;
    QPersistentModelIndex m_rootIndex;

    QLatin1String m_sep = QLatin1String(",");
    QLatin1String m_recordSep = QLatin1String("\r\n");
    QChar m_quote = QLatin1Char('"');
    QString m_escapedQuote = QLatin1String("\"\"");

    QString & formatField(QString & field) const;
};
