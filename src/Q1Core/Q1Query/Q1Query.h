#pragma once
#include "Q1Core/Q1Entity/Q1Relation.h"
#include "Q1Core/Q1Entity/Q1Table.h"
#include <algorithm>
#include <type_traits>
#include <QString>
#include <QList>
#include <QDebug>
#include <QMap>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <Q1Core/Q1Entity/Q1Column.h>

enum class Q1Operator { Equal, NotEqual, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual, Like };
enum class Q1Sort { Ascending, Descending };

template<typename Entity> class Q1Entity; // forward declaration

class TableDebugger
{
public:
    static void PrintTable(const QJsonArray& jsonArray, const QStringList& columnOrder = QStringList())
    {
        if (jsonArray.isEmpty())
        {
            qInfo().noquote() << "[QUERY] 0 rows";
            return;
        }

        QStringList headers;
        for (const QJsonValue& value : jsonArray)
        {
            const QStringList rowKeys = value.toObject().keys();
            for (const QString& key : rowKeys)
            {
                if (!headers.contains(key))
                    headers.append(key);
            }
        }

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
                QString data = FormatValue(obj.value(key));
                int dataLen = data.length() + 2;
                colWidths[key] = std::max(colWidths[key], dataLen);
            }
        }

        qInfo().noquote() << QStringLiteral("[QUERY] %1 row(s) | %2 column(s)")
                                 .arg(jsonArray.size())
                                 .arg(headers.size());

        QString separator = PrintSeparator(headers, colWidths);
        qInfo().noquote() << separator;

        QString headerRow = PrintRow(headers, headers, colWidths);
        qInfo().noquote() << headerRow;
        qInfo().noquote() << separator;

        for (const QJsonValue& val : jsonArray)
        {
            QJsonObject obj = val.toObject();
            QStringList row;
            for (const QString& key : headers)
            {
                row << FormatValue(obj.value(key));
            }
            qInfo().noquote() << PrintRow(row, headers, colWidths);
        }

        qInfo().noquote() << separator;
    }

private:
    static QString FormatValue(const QJsonValue& value)
    {
        if (value.isUndefined() || value.isNull())
            return QStringLiteral("<null>");
        if (value.isString())
            return value.toString();
        if (value.isBool())
            return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        if (value.isDouble())
            return QString::number(value.toDouble(), 'g', 15);
        if (value.isArray())
            return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
        if (value.isObject())
            return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
        return QString();
    }

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
        Invalidate();
        return *this;
    }

    // Query builders
    Q1Query& Where(const QString& column, Q1Operator op, const QVariant& value)
    {
        static const char* operators[] = {"=", "<>", ">", ">=", "<", "<=", "LIKE"};
        const QString identifier = repository ? repository->QuoteIdentifier(column) : column;
        if (value.isNull() && (op == Q1Operator::Equal || op == Q1Operator::NotEqual))
            return Where(identifier + (op == Q1Operator::Equal ? " IS NULL" : " IS NOT NULL"));
        QString name;
        do { name = QString(":q1_value_%1").arg(parameter_sequence++); } while (parameters.contains(name));
        Bind(name, value);
        return Where(QString("%1 %2 %3").arg(identifier, QString::fromLatin1(operators[static_cast<int>(op)]), name));
    }

    Q1Query& WhereEqual(const QString& column, const QVariant& value)
    { return Where(column, Q1Operator::Equal, value); }

    Q1Query& WhereRaw(const QString& clause) { return Where(clause); }

    Q1Query& OrderBy(const QString& column, Q1Sort direction)
    {
        const QString identifier = repository ? repository->QuoteIdentifier(column) : column;
        return OrderBy(identifier + (direction == Q1Sort::Ascending ? " ASC" : " DESC"));
    }

    Q1Query& Where(const QString& clause)
    {
        if (clause.trimmed().isEmpty())
            return *this;
        where_clause = where_clause.isEmpty()
                           ? clause
                           : QString("(%1) AND (%2)").arg(where_clause, clause);
        Invalidate();
        return *this;
    }

    Q1Query& OrWhere(const QString& clause)
    {
        if (clause.trimmed().isEmpty())
            return *this;
        where_clause = where_clause.isEmpty()
                           ? clause
                           : QString("(%1) OR (%2)").arg(where_clause, clause);
        Invalidate();
        return *this;
    }

    Q1Query& Bind(const QString& name, const QVariant& value)
    {
        parameters.insert(name, value);
        Invalidate();
        return *this;
    }

    Q1Query& OrderBy(const QString& clause)
    {
        order_by = clause;
        Invalidate();
        return *this;
    }

    Q1Query& Limit(int l)
    {
        limit_val = l;
        Invalidate();
        return *this;
    }

    Q1Query& Take(int count)
    {
        limit_val = count;
        Invalidate();
        return *this;
    }

    Q1Query& Skip(int count)
    {
        offset_val = qMax(0, count);
        Invalidate();
        return *this;
    }

    Q1Query& OrderByAsc(const QString& column)
    {
        if (!order_by.isEmpty())
        {
            order_by += ", ";
        }
        order_by += QString("%1 ASC").arg(column);
        Invalidate();
        return *this;
    }

    Q1Query& OrderByDesc(const QString& column)
    {
        if (!order_by.isEmpty())
        {
            order_by += ", ";
        }
        order_by += QString("%1 DESC").arg(column);
        Invalidate();
        return *this;
    }

    // Join methods
    Q1Query& InnerJoin(const QString& table, const QString& onCondition)
    {
        joins += QString(" INNER JOIN %1 ON %2").arg(table, onCondition);
        Invalidate();
        return *this;
    }

    Q1Query& LeftJoin(const QString& table, const QString& onCondition)
    {
        joins += QString(" LEFT JOIN %1 ON %2").arg(table, onCondition);
        Invalidate();
        return *this;
    }

    Q1Query& RightJoin(const QString& table, const QString& onCondition)
    {
        joins += QString(" RIGHT JOIN %1 ON %2").arg(table, onCondition);
        Invalidate();
        return *this;
    }

    Q1Query& FullJoin(const QString& table, const QString& onCondition)
    {
        joins += QString(" FULL OUTER JOIN %1 ON %2").arg(table, onCondition);
        Invalidate();
        return *this;
    }

    // Select with optional columns - works for both Include and Joins
    Q1Query& Select(const QStringList& columns = QStringList())
    {
        selected_columns = columns;
        Invalidate();
        return *this;
    }

    Q1Query& GroupBy(const QString& columns)
    {
        group_by = columns;
        Invalidate();
        return *this;
    }

    Q1Query& Having(const QString& condition)
    {
        having_clause = condition;
        Invalidate();
        return *this;
    }

    Q1Query& Include(const QString& relationship_name)
    {
        included_relations.append(relationship_name);
        Invalidate();
        return *this;
    }

    Q1Query& Include(const QStringList& relationshipNames)
    {
        included_relations.append(relationshipNames);
        Invalidate();
        return *this;
    }

    Q1Query& SetColumns(const QStringList& columns)
    {
        selected_columns = columns;
        Invalidate();
        return *this;
    }

    // Display methods
    bool Any()
    {
        Limit(1);
        return !ToList().isEmpty();
    }

    Entity First()
    {
        Limit(1);
        const QList<Entity> rows = ToList();
        return rows.isEmpty() ? Entity{} : rows.first();
    }

    QList<Entity> ShowList()
    {
        if (!repository)
        {
            return results;
        }

        if (!executed)
        {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause,
                                             parameters,
                                             offset_val,
                                             distinct_flag);
            executed = true;
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
            qInfo().noquote() << "[QUERY] 0 rows";
        }

        return results;
    }

    QList<Entity> ShowJson()
    {
        if (!repository)
        {
            return results;
        }

        if (!executed)
        {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause,
                                             parameters,
                                             offset_val,
                                             distinct_flag);
            executed = true;
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
            qInfo().noquote() << "[QUERY] JSON result:";
            qInfo().noquote() << QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
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

        if (!executed)
        {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause,
                                             parameters,
                                             offset_val,
                                             distinct_flag);
            executed = true;

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
        if (!executed)
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
                                             having_clause,
                                             parameters,
                                             offset_val,
                                             distinct_flag);
            executed = true;

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
    void Invalidate()
    {
        executed = false;
        results.clear();
        relation_cache.clear();
    }

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
            sortedArray.append(SortJsonValue(val));
        return sortedArray;
    }

    QJsonValue SortJsonValue(const QJsonValue& value)
    {
        if (value.isArray())
        {
            QJsonArray sorted;
            for (const QJsonValue& item : value.toArray())
                sorted.append(SortJsonValue(item));
            return sorted;
        }

        if (value.isObject())
        {
            const QJsonObject object = value.toObject();
            QStringList keys = object.keys();
            std::sort(keys.begin(), keys.end());

            QJsonObject sorted;
            for (const QString& key : keys)
                sorted.insert(key, SortJsonValue(object.value(key)));
            return sorted;
        }

        return value;
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
        qInfo().noquote() << "[QUERY] JSON result with includes:";
        qInfo().noquote() << QString::fromUtf8(
            QJsonDocument(SortJsonKeys(jsonArray)).toJson(QJsonDocument::Indented));
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

        QString sql = QString("SELECT %1 AS __agg FROM %2")
                          .arg(selectExpr, repository->QuoteIdentifier(repository->GetTable().table_name));

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

        QVariant result = repository->ExecuteScalar(sql, parameters);
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

        const Q1Table& table = repository->GetTable();
        const bool relationUsesLocalForeignKey = table.HasColumn(relation.foreign_key);
        const QString sourceColumn = relationUsesLocalForeignKey ? relation.foreign_key : relation.reference_key;
        const QString targetColumn = relationUsesLocalForeignKey ? relation.reference_key : relation.foreign_key;

        QVariantList keyValues;
        for (const Entity& entity : entities)
        {
            QVariant keyValue = GetPropertyValue(entity, sourceColumn);
            if (keyValue.isValid() && !keyValues.contains(keyValue))
            {
                keyValues.append(keyValue);
            }
        }

        if (keyValues.isEmpty())
        {
            return;
        }

        QStringList placeholders;
        QVariantMap relationParameters;
        for (int i = 0; i < keyValues.size(); ++i)
        {
            const QString name = QString(":q1_relation_%1").arg(i);
            placeholders.append(name);
            relationParameters.insert(name, keyValues.at(i));
        }

        QString query = QString("SELECT * FROM %1 WHERE %2 IN (%3)")
                            .arg(repository->QuoteIdentifier(relation.top_table),
                                 repository->QuoteIdentifier(targetColumn),
                                 placeholders.join(", "));

        qInfo().noquote() << QStringLiteral("[QUERY] INCLUDE %1 -> %2 related key(s)")
                                 .arg(relation.top_table)
                                 .arg(keyValues.size());
        qDebug() << "Eager Loading Query:" << query;

        QList<QJsonObject> relatedData = repository->ExecuteRelationQuery(query, relationParameters);
        relation_cache[relation.top_table] = relatedData;
    }

    QVariant GetPropertyValue(const Entity& entity, const QString& columnName)
    {
        const QMap<QString, typename Q1Entity<Entity>::PropertyInfo>& propMap = repository->GetPropertyMap();

        auto it = propMap.find(columnName);
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
                return v;
            }
            case BIGINT:
            {
                qint64 v = *reinterpret_cast<const qint64*>(memberPtr);
                return v;
            }
            case VARCHAR:
            case TEXT:
            case CHAR:
            {
                const QString* s = reinterpret_cast<const QString*>(memberPtr);
                if (!s)
                    return QVariant();

                return *s;
            }
            default:
                return QVariant();
            }
        }

        return QVariant();
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

                    const bool relationUsesLocalForeignKey = repository->GetTable().HasColumn(matchingRelation->foreign_key);
                    const QString localColumn = relationUsesLocalForeignKey
                                                    ? matchingRelation->foreign_key
                                                    : matchingRelation->reference_key;
                    const QString relatedColumn = relationUsesLocalForeignKey
                                                      ? matchingRelation->reference_key
                                                      : matchingRelation->foreign_key;
                    const QString localValue = obj[localColumn].toVariant().toString();

                    QJsonArray relatedArray;
                    for (const QJsonObject& related : relatedData)
                    {
                        QString relatedValue = related[relatedColumn].toVariant().toString();

                        if (localValue == relatedValue)
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
    int offset_val = 0;
    QList<Entity> results;
    bool distinct_flag;
    QStringList included_relations;
    QMap<QString, QList<QJsonObject>> relation_cache;
    QVariantMap parameters;
    bool executed = false;
    int parameter_sequence = 0;
};
