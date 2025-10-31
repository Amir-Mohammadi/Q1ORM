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


    //aggregate functions
    template<typename T = double>
    T Max(const QString& column)
    {
        return ExecuteAggregate<T>(QString("MAX(%1)").arg(column));
    }

    template<typename T = double>
    T Min(const QString& column)
    {
        return ExecuteAggregate<T>(QString("MIN(%1)").arg(column));
    }

    template<typename T = int>
    T Count(const QString& column = "*")
    {
        return ExecuteAggregate<T>(QString("COUNT(%1)").arg(column));
    }

    template<typename T = double>
    T Sum(const QString& column)
    {
        return ExecuteAggregate<T>(QString("SUM(%1)").arg(column));
    }

    template<typename T = double>
    T Avg(const QString& column)
    {
        return ExecuteAggregate<T>(QString("AVG(%1)").arg(column));
    }

    Q1Query& Distinct()
    {
        distinct_flag = true;
        return *this;
    }

    // Query builders
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

    // Join methods
    Q1Query& InnerJoin(const QString& table, const QString& onCondition)
    {
        joins += QString(" INNER JOIN %1 ON %2").arg(table, onCondition);
        return *this;
    }

    Q1Query& LeftJoin(const QString& table, const QString& onCondition)
    {
        joins += QString(" LEFT JOIN %1 ON %2").arg(table, onCondition);
        return *this;
    }

    Q1Query& RightJoin(const QString& table, const QString& onCondition)
    {
        joins += QString(" RIGHT JOIN %1 ON %2").arg(table, onCondition);
        return *this;
    }

    Q1Query& FullJoin(const QString& table, const QString& onCondition)
    {
        joins += QString(" FULL OUTER JOIN %1 ON %2").arg(table, onCondition);
        return *this;
    }

    // Select specific columns
    Q1Query& Select(const QStringList& columns)
    {
        selected_columns = columns;
        return *this;
    }

    // Group and Having
    Q1Query& GroupBy(const QString& columns)
    {
        group_by = columns;
        return *this;
    }

    Q1Query& Having(const QString& condition)
    {
        having_clause = condition;
        return *this;
    }


    Q1Query& SetColumns(const QStringList& columns)
    {
        selected_columns = columns;
        return *this;
    }


    QList<Entity>& ShowList()
    {
        if (!repository) return results;

        if (results.isEmpty())
                ToList();


        QJsonArray array = repository->GetLastJson();
        if (!array.isEmpty()) {
            QStringList columnOrder = array[0].toObject().keys();
            TableDebugger::PrintTable(array, columnOrder);
        }

        return results;
    }



    QList<Entity>& ShowJson()
    {
        if (!repository) return results;

        if (results.isEmpty())
            ToList();

        QJsonArray array = repository->GetLastJson();

        QJsonArray sortedArray;
        for (const auto& val : array) {
            QJsonObject obj = val.toObject();
            QJsonObject sortedObj;

            QStringList keys = obj.keys();
            std::sort(keys.begin(), keys.end());

            for (const QString& key : keys) {
                sortedObj.insert(key, obj[key]);
            }

            sortedArray.append(sortedObj);
        }

        QJsonDocument doc(sortedArray);
        qDebug().noquote() << doc.toJson(QJsonDocument::Indented);

        return results;


    }

    // Operators
    operator QList<Entity>()
    {
        return ToList();
    }


    QList<Entity> ToList()
    {
        if (!repository) return {};

        if (results.isEmpty()) {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause);
        }

        return results;
    }


    QByteArray ToJson()
    {
        if (results.isEmpty()) {
            if (!repository) return QByteArray();
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause);
        }


        QJsonArray array = repository->GetLastJson();


        QJsonArray sortedArray;
        for (const auto& val : array) {
            QJsonObject obj = val.toObject();
            QJsonObject sortedObj;

            QStringList keys = obj.keys();
            std::sort(keys.begin(), keys.end());

            for (const QString& key : keys) {
                sortedObj.insert(key, obj[key]);
            }

            sortedArray.append(sortedObj);
        }

        QJsonDocument doc(sortedArray);
        return doc.toJson(QJsonDocument::Indented);

    }


private:
    template<typename T>
    T ExecuteAggregate(const QString& function)
    {
        if(!repository) return T();

        QString sql = "SELECT ";
        if (distinct_flag)
            sql += "DISTINCT ";

        if (selected_columns.isEmpty())
            sql += "*";
        else
            sql += selected_columns.join(", ");

        sql += " FROM " + repository->GetTable().table_name;

        if (!joins.isEmpty())
            sql += " " + joins;
        if (!where_clause.isEmpty())
            sql += " WHERE " + where_clause;
        if (!group_by.isEmpty())
            sql += " GROUP BY " + group_by;
        if (!having_clause.isEmpty())
            sql += " HAVING " + having_clause;

        QVariant result = repository->ExecuteScalar(sql);
        return result.value<T>();
    }

private:
    Q1Entity<Entity>* repository = nullptr;
    QString where_clause;
    QString order_by;
    QString joins;
    QString group_by;
    QString having_clause;
    QStringList selected_columns;
    int limit_val;
    QList<Entity> results;
    bool distinct_flag = false;
};
