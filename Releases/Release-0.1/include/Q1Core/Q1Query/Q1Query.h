#pragma once
#include <QString>
#include <QList>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>





template<typename Entity> class Q1Entity; // forward
class TableDebugger
{
public:
    static void PrintTable(const QJsonArray& jsonArray, const QStringList& columnOrder = QStringList())
    {
        if (jsonArray.isEmpty()) {
            qDebug() << "Table is empty";
            return;
        }

        // Get all keys from first object
        QJsonObject firstObj = jsonArray[0].toObject();
        QStringList headers = firstObj.keys();

        // Use provided column order if available, otherwise sort alphabetically
        if (!columnOrder.isEmpty()) {
            QStringList orderedHeaders;
            for (const QString& col : columnOrder) {
                if (headers.contains(col)) {
                    orderedHeaders << col;
                }
            }
            // Add any remaining headers not in columnOrder
            for (const QString& header : headers) {
                if (!orderedHeaders.contains(header)) {
                    orderedHeaders << header;
                }
            }
            headers = orderedHeaders;
        } else {
            std::sort(headers.begin(), headers.end());
        }

        // Calculate column widths (ensure minimum of header width)
        QMap<QString, int> colWidths;
        for (const QString& header : headers) {
            colWidths[header] = header.length() + 2;  // Add padding
        }

        // Update widths based on data
        for (const QJsonValue& val : jsonArray) {
            QJsonObject obj = val.toObject();
            for (const QString& key : headers) {
                QString data = obj[key].toVariant().toString();
                int dataLen = data.length() + 2;  // Add padding
                colWidths[key] = std::max(colWidths[key], dataLen);
            }
        }

        // Print separator
        QString separator = PrintSeparator(headers, colWidths);
        qDebug().noquote() << separator;

        // Print header
        QString headerRow = PrintRow(headers, headers, colWidths);
        qDebug().noquote() << headerRow;

        // Print separator
        qDebug().noquote() << separator;

        // Print data rows
        for (const QJsonValue& val : jsonArray) {
            QJsonObject obj = val.toObject();
            QStringList row;
            for (const QString& key : headers) {
                row << obj[key].toVariant().toString();
            }
            qDebug().noquote() << PrintRow(row, headers, colWidths);
        }

        // Print separator
        qDebug().noquote() << separator;
        qDebug() << "Total rows:" << jsonArray.size();
    }

private:
    static QString PrintRow(const QStringList& data, const QStringList& headers, const QMap<QString, int>& colWidths)
    {
        QString row = "| ";
        for (int i = 0; i < data.size(); ++i) {
            int width = colWidths.value(headers[i], 10);
            row += QString("%1").arg(data[i], -width) + "| ";
        }
        return row;
    }

    static QString PrintSeparator(const QStringList& headers, const QMap<QString, int>& colWidths)
    {
        QString sep = "+";
        for (const QString& header : headers) {
            int width = colWidths.value(header, 10);
            sep += QString("").fill('-', width) + "+";
        }
        return sep;
    }
};

template<typename Entity>
class Q1Query
{
public:
    explicit Q1Query(Q1Entity<Entity>* repo = nullptr)
        : repository(repo), limit_val(-1) {}


    Q1Query& Where(const QString& clause)
    {
        where_clause = clause;
        return *this;
    }

    Q1Query& OrderBy(const QString& clause)
    {
        order_by = clause;
        return *this;
    }

    Q1Query& Limit(int l)
    {
        limit_val = l;
        return *this;
    }


    Q1Query& OrderByAsc(const QString& column)
    {
        if(!order_by.isEmpty())
            order_by += ", ";
        order_by += QString("%1 ASC").arg(column);
        return *this;
    }

    Q1Query& OrderByDesc(const QString& column)
    {
        if(!order_by.isEmpty())
            order_by += ", ";
        order_by += QString("%1 DESC").arg(column);
        return *this;
    }

    operator QList<Entity>()
    {
        return ToList();
    }

    QList<Entity> ToList()
    {
        if (!repository) return {};

        // Only execute query if results are empty
        if (results.isEmpty()) {
            results = repository->SelectExec(where_clause, order_by, limit_val);
        }

        // Print table only when ToList() is called
        if (!results.isEmpty()) {
            QJsonArray array;
            QStringList columnOrder;

            for (const auto& entity : results)
            {
                QJsonObject obj = repository->EntityToJson(entity);

                // Capture column order from first entity
                if (columnOrder.isEmpty()) {
                    columnOrder = obj.keys();
                }

                array.append(obj);
            }

            TableDebugger::PrintTable(array, columnOrder);
        }
        return results;
    }

    // Return JSON for the same result with sorted keys
    QByteArray ToJson()
    {
        if (results.isEmpty()) {
            if (!repository) return QByteArray();
            results = repository->SelectExec(where_clause, order_by, limit_val);
        }

        QJsonArray array;
        for (const auto& entity : results)
        {
            QJsonObject obj = repository->EntityToJson(entity);
            QJsonObject sortedObj;

            // Sort keys alphabetically
            QStringList keys = obj.keys();
            std::sort(keys.begin(), keys.end());

            for (const QString& key : keys) {
                sortedObj.insert(key, obj[key]);
            }

            array.append(sortedObj);
        }

        // Return pretty JSON as QByteArray
        QJsonDocument doc(array);
        return doc.toJson(QJsonDocument::Indented);
    }

    void DebugTable()
    {
        if (results.isEmpty()) ToList();

        QJsonArray array;
        QStringList columnOrder;

        for (const auto& entity : results)
        {
            QJsonObject obj = repository->EntityToJson(entity);

            // Capture column order from first entity
            if (columnOrder.isEmpty()) {
                columnOrder = obj.keys();
            }

            array.append(obj);
        }

        TableDebugger::PrintTable(array, columnOrder);
    }

private:
    Q1Entity<Entity>* repository = nullptr;
    QString where_clause;
    QString order_by;
    int limit_val;
    QList<Entity> results;
    bool show_debug;
};
