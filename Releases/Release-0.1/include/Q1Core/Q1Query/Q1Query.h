#pragma once
#include "Q1Core/Q1Entity/Q1Relation.h"
#include "Q1Core/Q1Entity/Q1Table.h"
#include <QString>
#include <QList>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <Q1Core/Q1Entity/Q1Column.h>

template<typename Entity> class Q1Entity; // forward declaration

class TableDebugger
{
public:
    static void PrintTable(const QJsonArray& jsonArray, const QStringList& columnOrder = QStringList())
    {
        if (jsonArray.isEmpty())
        {
            qDebug() << "Table is empty";
            return;
        }

        QJsonObject firstObj = jsonArray[0].toObject();
        QStringList headers = firstObj.keys();

        if (!columnOrder.isEmpty())
        {
            QStringList orderedHeaders;
            for (const QString& col : columnOrder)
            {
                if (headers.contains(col))
                {
                    orderedHeaders << col;
                }
            }

            for (const QString& header : headers)
            {
                if (!orderedHeaders.contains(header))
                {
                    orderedHeaders << header;
                }
            }
            headers = orderedHeaders;
        }
        else
        {
            std::sort(headers.begin(), headers.end());
        }

        QMap<QString, int> colWidths;
        for (const QString& header : headers)
        {
            colWidths[header] = header.length() + 2;
        }

        for (const QJsonValue& val : jsonArray)
        {
            QJsonObject obj = val.toObject();
            for (const QString& key : headers)
            {
                QString data = obj[key].toVariant().toString();
                int dataLen = data.length() + 2;
                colWidths[key] = std::max(colWidths[key], dataLen);
            }
        }

        QString separator = PrintSeparator(headers, colWidths);
        qDebug().noquote() << separator;

        QString headerRow = PrintRow(headers, headers, colWidths);
        qDebug().noquote() << headerRow;
        qDebug().noquote() << separator;

        for (const QJsonValue& val : jsonArray)
        {
            QJsonObject obj = val.toObject();
            QStringList row;
            for (const QString& key : headers)
            {
                row << obj[key].toVariant().toString();
            }
            qDebug().noquote() << PrintRow(row, headers, colWidths);
        }

        qDebug().noquote() << separator;
        qDebug() << "Total rows:" << jsonArray.size();
    }

private:
    static QString PrintRow(const QStringList& data, const QStringList& headers, const QMap<QString, int>& colWidths)
    {
        QString row = "| ";
        for (int i = 0; i < data.size(); ++i)
        {
            int width = colWidths.value(headers[i], 10);
            row += QString("%1").arg(data[i], -width) + "| ";
        }
        return row;
    }

    static QString PrintSeparator(const QStringList& headers, const QMap<QString, int>& colWidths)
    {
        QString sep = "+";
        for (const QString& header : headers)
        {
            int width = colWidths.value(header, 10);
            sep += QString(width, '-') + "+";
        }
        return sep;
    }
};

template<typename Entity>
class Q1Query
{
public:
    explicit Q1Query(Q1Entity<Entity>* repo = nullptr)
        : repository(repo), limit_val(-1), distinct_flag(false) {}

    // Aggregate functions
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
        if (!order_by.isEmpty())
        {
            order_by += ", ";
        }
        order_by += QString("%1 ASC").arg(column);
        return *this;
    }

    Q1Query& OrderByDesc(const QString& column)
    {
        if (!order_by.isEmpty())
        {
            order_by += ", ";
        }
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

    // Select with optional columns - works for both Include and Joins
    Q1Query& Select(const QStringList& columns = QStringList())
    {
        selected_columns = columns;
        return *this;
    }

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

    Q1Query& Include(const QString& relationship_name)
    {
        included_relations.append(relationship_name);
        return *this;
    }

    Q1Query& Include(const QStringList& relationshipNames)
    {
        included_relations.append(relationshipNames);
        return *this;
    }

    Q1Query& SetColumns(const QStringList& columns)
    {
        selected_columns = columns;
        return *this;
    }

    // Display methods
    QList<Entity>& ShowList()
    {
        if (!repository)
        {
            return results;
        }

        if (results.isEmpty())
        {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause);
        }

        QJsonArray array = repository->GetLastJson();

        if (!included_relations.isEmpty())
        {
            LoadRelatedData(results);
            array = AppendRelatedDataToJson(array);
            array = FlattenAllIncludedData(array);
        }
        else if (!joins.isEmpty() && selected_columns.isEmpty())
        {
            array = AutoPrefixJoinedColumns(array);
        }

        if (!array.isEmpty())
        {
            QStringList columnOrder = array[0].toObject().keys();
            TableDebugger::PrintTable(array, columnOrder);
        }
        else
        {
            qDebug() << "No results found";
        }

        return results;
    }

    QList<Entity>& ShowJson()
    {
        if (!repository)
        {
            return results;
        }

        if (results.isEmpty())
        {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause);
        }

        QJsonArray array = repository->GetLastJson();

        if (!included_relations.isEmpty())
        {
            LoadRelatedData(results);
            array = AppendRelatedDataToJson(array);
            PrintJsonWithIncludes(array);
        }
        else
        {
            QJsonArray sortedArray = SortJsonKeys(array);
            QJsonDocument doc(sortedArray);
            qDebug().noquote() << doc.toJson(QJsonDocument::Indented);
        }

        return results;
    }

    // Conversion operators
    operator QList<Entity>()
    {
        return ToList();
    }

    QList<Entity> ToList()
    {
        if (!repository)
        {
            return {};
        }

        if (results.isEmpty())
        {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause);

            if (!included_relations.isEmpty())
            {
                LoadRelatedData(results);
                QJsonArray array = repository->GetLastJson();
                array = AppendRelatedDataToJson(array);
                repository->SetLastJson(array);
            }
        }

        return results;
    }

    QByteArray ToJson()
    {
        if (results.isEmpty())
        {
            if (!repository)
            {
                return QByteArray();
            }

            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause);

            if (!included_relations.isEmpty())
            {
                LoadRelatedData(results);
            }
        }

        QJsonArray array = repository->GetLastJson();

        if (!included_relations.isEmpty())
        {
            array = AppendRelatedDataToJson(array);
        }

        QJsonArray sortedArray = SortJsonKeys(array);
        QJsonDocument doc(sortedArray);
        return doc.toJson(QJsonDocument::Indented);
    }

private:
    // Helper methods
    QJsonArray AutoPrefixJoinedColumns(const QJsonArray& array)
    {
        if (!array.isEmpty())
        {
            qWarning() << "Warning: Using joins without Select() may cause column name conflicts.";
            qWarning() << "Recommendation: Use .Select() with aliases, e.g.:";
            qWarning() << "  .Select({\"cities.id AS city_id\", \"cities.name AS city_name\",";
            qWarning() << "           \"countries.id AS country_id\", \"countries.name AS country_name\"})";
        }
        return array;
    }

    QJsonArray SortJsonKeys(const QJsonArray& array)
    {
        QJsonArray sortedArray;
        for (const auto& val : array)
        {
            QJsonObject obj = val.toObject();
            QJsonObject sortedObj;

            QStringList keys = obj.keys();
            std::sort(keys.begin(), keys.end());

            for (const QString& key : keys)
            {
                sortedObj.insert(key, obj[key]);
            }

            sortedArray.append(sortedObj);
        }
        return sortedArray;
    }

    QJsonArray FlattenAllIncludedData(const QJsonArray& jsonArray)
    {
        QJsonArray flatArray;

        for (const auto& val : jsonArray)
        {
            QJsonObject obj = val.toObject();
            QJsonObject baseObj;
            QMap<QString, QJsonArray> includeArrays;

            for (const QString& key : obj.keys())
            {
                if (included_relations.contains(key) && obj[key].isArray())
                {
                    includeArrays[key] = obj[key].toArray();
                }
                else
                {
                    baseObj.insert(key, obj[key]);
                }
            }

            if (includeArrays.isEmpty())
            {
                flatArray.append(baseObj);
                continue;
            }

            QList<QJsonObject> expandedRows = ExpandIncludes(baseObj, includeArrays);

            if (expandedRows.isEmpty())
            {
                flatArray.append(baseObj);
            }
            else
            {
                for (const QJsonObject& row : expandedRows)
                {
                    flatArray.append(row);
                }
            }
        }

        return flatArray;
    }

    QList<QJsonObject> ExpandIncludes(const QJsonObject& baseObj, const QMap<QString, QJsonArray>& includeArrays)
    {
        QList<QJsonObject> result;

        if (includeArrays.isEmpty())
        {
            result.append(baseObj);
            return result;
        }

        QString firstInclude = includeArrays.firstKey();
        QJsonArray firstArray = includeArrays[firstInclude];

        if (firstArray.isEmpty())
        {
            result.append(baseObj);
        }
        else
        {
            for (const auto& relatedVal : firstArray)
            {
                QJsonObject relatedObj = relatedVal.toObject();
                QJsonObject mergedObj = baseObj;

                for (const QString& key : relatedObj.keys())
                {
                    QString prefixedKey = firstInclude + "_" + key;
                    mergedObj.insert(prefixedKey, relatedObj[key]);
                }

                result.append(mergedObj);
            }
        }

        return result;
    }

    void PrintJsonWithIncludes(const QJsonArray& jsonArray)
    {
        qDebug().noquote() << "[";

        for (int i = 0; i < jsonArray.size(); ++i)
        {
            QJsonObject obj = jsonArray[i].toObject();
            QJsonObject baseObj;
            QMap<QString, QJsonArray> includeData;

            for (const QString& key : obj.keys())
            {
                if (included_relations.contains(key))
                {
                    if (obj[key].isArray())
                    {
                        includeData[key] = obj[key].toArray();
                    }
                }
                else
                {
                    baseObj.insert(key, obj[key]);
                }
            }

            QJsonObject sortedBase;
            QStringList keys = baseObj.keys();
            std::sort(keys.begin(), keys.end());
            for (const QString& key : keys)
            {
                sortedBase.insert(key, baseObj[key]);
            }

            qDebug().noquote() << "  {";

            QStringList baseKeys = sortedBase.keys();
            for (int j = 0; j < baseKeys.size(); ++j)
            {
                QString key = baseKeys[j];
                QJsonValue val = sortedBase[key];
                QString comma = (j < baseKeys.size() - 1 || !includeData.isEmpty()) ? "," : "";

                if (val.isString())
                {
                    qDebug().noquote() << QString("    \"%1\": \"%2\"%3").arg(key, val.toString(), comma);
                }
                else if (val.isDouble())
                {
                    qDebug().noquote() << QString("    \"%1\": %2%3").arg(key, QString::number(val.toDouble()), comma);
                }
                else if (val.isBool())
                {
                    qDebug().noquote() << QString("    \"%1\": %2%3").arg(key, val.toBool() ? "true" : "false", comma);
                }
                else if (val.isNull())
                {
                    qDebug().noquote() << QString("    \"%1\": null%2").arg(key, comma);
                }
                else
                {
                    qDebug().noquote() << QString("    \"%1\": %2%3").arg(key, QString::number(val.toDouble()), comma);
                }
            }

            QStringList includeKeys = includeData.keys();
            for (int j = 0; j < includeKeys.size(); ++j)
            {
                QString relName = includeKeys[j];
                QJsonArray relArray = includeData[relName];
                QString comma = (j < includeKeys.size() - 1) ? "," : "";

                qDebug().noquote() << QString("    \"%1\": [").arg(relName);

                for (int k = 0; k < relArray.size(); ++k)
                {
                    QJsonObject relObj = relArray[k].toObject();
                    QString relComma = (k < relArray.size() - 1) ? "," : "";

                    QJsonObject sortedRel;
                    QStringList relObjKeys = relObj.keys();
                    std::sort(relObjKeys.begin(), relObjKeys.end());
                    for (const QString& key : relObjKeys)
                    {
                        sortedRel.insert(key, relObj[key]);
                    }

                    QJsonDocument relDoc(sortedRel);
                    QString relJson = relDoc.toJson(QJsonDocument::Compact);
                    qDebug().noquote() << QString("      %1%2").arg(relJson, relComma);
                }

                qDebug().noquote() << QString("    ]%1").arg(comma);
            }

            QString rowComma = (i < jsonArray.size() - 1) ? "," : "";
            qDebug().noquote() << QString("  }%1").arg(rowComma);
        }

        qDebug().noquote() << "]";
    }

    template<typename T>
    T ExecuteAggregate(const QString& function)
    {
        if (!repository)
        {
            return T();
        }

        QString selectExpr = function;
        if (distinct_flag)
        {
            int paren = selectExpr.indexOf('(');
            if (paren >= 0)
            {
                selectExpr.insert(paren + 1, "DISTINCT ");
            }
            else
            {
                selectExpr = "DISTINCT " + selectExpr;
            }
        }

        QString sql = QString("SELECT %1 AS __agg FROM \"%2\"")
                          .arg(selectExpr, repository->GetTable().table_name);

        if (!joins.isEmpty())
        {
            sql += " " + joins;
        }
        if (!where_clause.isEmpty())
        {
            sql += " WHERE " + where_clause;
        }
        if (!group_by.isEmpty())
        {
            sql += " GROUP BY " + group_by;
        }
        if (!having_clause.isEmpty())
        {
            sql += " HAVING " + having_clause;
        }

        QVariant result = repository->ExecuteScalar(sql);
        if (!result.isValid() || result.isNull())
        {
            return T();
        }

        if constexpr (std::is_same_v<T, int>)
        {
            return static_cast<T>(result.toInt());
        }
        else if constexpr (std::is_same_v<T, qint64>)
        {
            return static_cast<T>(result.toLongLong());
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            return static_cast<T>(result.toDouble());
        }
        else
        {
            return result.value<T>();
        }
    }

    void LoadRelatedData(QList<Entity>& entities)
    {
        if (!repository)
        {
            return;
        }

        for (const QString& relationName : included_relations)
        {
            LoadRelation(entities, relationName);
        }
    }

    void LoadRelation(QList<Entity>& entities, const QString& relationName)
    {
        if (!repository)
        {
            return;
        }

        const Q1Table& table = repository->GetTable();

        for (const Q1Relation& relation : table.relations)
        {
            if (relation.top_table == relationName)
            {
                LoadRelationData(entities, relation);
                break;
            }
        }
    }

    void LoadRelationData(QList<Entity>& entities, const Q1Relation& relation)
    {
        if (entities.isEmpty() || !repository)
        {
            return;
        }

        QStringList fkValues;
        for (const Entity& entity : entities)
        {
            QString fkValue = GetForeignKeyValue(entity, relation.foreign_key);
            if (!fkValue.isEmpty() && !fkValues.contains(fkValue))
            {
                fkValues.append(fkValue);
            }
        }

        if (fkValues.isEmpty())
        {
            return;
        }

        QString query = QString("SELECT * FROM \"%1\" WHERE \"%2\" IN (%3)")
                            .arg(relation.top_table,
                                 relation.reference_key,
                                 fkValues.join(", "));

        qDebug() << "Eager Loading Query:" << query;

        QList<QJsonObject> relatedData = repository->ExecuteRelationQuery(query);
        relation_cache[relation.top_table] = relatedData;
    }

    QString GetForeignKeyValue(const Entity& entity, const QString& fkColumn)
    {
        const QMap<QString, typename Q1Entity<Entity>::PropertyInfo>& propMap = repository->GetPropertyMap();

        auto it = propMap.find(fkColumn);
        if (it != propMap.end())
        {
            const typename Q1Entity<Entity>::PropertyInfo& info = it.value();
            const char* memberPtr = reinterpret_cast<const char*>(&entity) + info.offset;

            switch (info.type)
            {
            case INTEGER:
            case SMALLINT:
            {
                int v = *reinterpret_cast<const int*>(memberPtr);
                return QString::number(v);
            }
            case BIGINT:
            {
                qint64 v = *reinterpret_cast<const qint64*>(memberPtr);
                return QString::number(v);
            }
            case VARCHAR:
            case TEXT:
            case CHAR:
            {
                const QString* s = reinterpret_cast<const QString*>(memberPtr);
                return s ? *s : QString();
            }
            default:
                return QString();
            }
        }

        return QString();
    }

    QJsonArray AppendRelatedDataToJson(const QJsonArray& originalArray)
    {
        QJsonArray result = originalArray;

        for (int i = 0; i < result.size(); ++i)
        {
            QJsonObject obj = result[i].toObject();

            for (const auto& relationName : included_relations)
            {
                if (relation_cache.contains(relationName))
                {
                    QList<QJsonObject> relatedData = relation_cache[relationName];

                    const Q1Table& table = repository->GetTable();
                    const Q1Relation* matchingRelation = nullptr;

                    for (const Q1Relation& rel : table.relations)
                    {
                        if (rel.top_table == relationName)
                        {
                            matchingRelation = &rel;
                            break;
                        }
                    }

                    if (!matchingRelation)
                    {
                        continue;
                    }

                    QString fkValue = obj[matchingRelation->foreign_key].toVariant().toString();

                    QJsonArray relatedArray;
                    for (const QJsonObject& related : relatedData)
                    {
                        QString refValue = related[matchingRelation->reference_key].toVariant().toString();

                        if (fkValue == refValue)
                        {
                            relatedArray.append(related);
                        }
                    }

                    obj.insert(relationName, relatedArray);
                }
            }

            result[i] = obj;
        }

        return result;
    }

private:
    Q1Entity<Entity>* repository;
    QString where_clause;
    QString order_by;
    QString joins;
    QString group_by;
    QString having_clause;
    QStringList selected_columns;
    int limit_val;
    QList<Entity> results;
    bool distinct_flag;
    QStringList included_relations;
    QMap<QString, QList<QJsonObject>> relation_cache;
};
