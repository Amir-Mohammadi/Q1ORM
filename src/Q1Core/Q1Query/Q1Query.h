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



    QList<Entity>& ShowList()
    {
        if (!repository) return results;

        // Execute query if not already done
        if (results.isEmpty()) {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause);
        }

        // Load related data FIRST if Include() was called
        if (!included_relations.isEmpty()) {
            LoadRelatedData(results);
        }

        // Get JSON array with base data
        QJsonArray array = repository->GetLastJson();

        // Append eager loaded related data to JSON
        if (!included_relations.isEmpty()) {
            array = AppendRelatedDataToJson(array);

            // Flatten array for table display (expand included records)
            array = FlattenIncludedData(array);
        }

        // Print table with all data flattened
        if (!array.isEmpty()) {
            QStringList columnOrder = array[0].toObject().keys();
            TableDebugger::PrintTable(array, columnOrder);
        }

        return results;
    }


    QList<Entity>& ShowJson()
    {
        if (!repository) return results;

        // Execute query if not already done
        if (results.isEmpty()) {
            results = repository->SelectExec(where_clause,
                                             order_by,
                                             limit_val,
                                             joins,
                                             selected_columns,
                                             group_by,
                                             having_clause);
        }

        // Load related data FIRST if Include() was called
        if (!included_relations.isEmpty()) {
            LoadRelatedData(results);
        }

        // Get JSON array with base data
        QJsonArray array = repository->GetLastJson();

        if (!included_relations.isEmpty()) {
            // Append eager loaded related data
            array = AppendRelatedDataToJson(array);

            // Print with nested format (don't flatten)
            PrintJsonWithIncludes(array);
        } else {
            // Regular JSON output for non-included queries
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
        }

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


            if (!included_relations.isEmpty()) {
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
        if (results.isEmpty()) {
            if (!repository) return QByteArray();
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

    QJsonArray FlattenIncludedData(const QJsonArray& jsonArray)
    {
        QJsonArray flatArray;

        for (const auto& val : jsonArray) {
            QJsonObject obj = val.toObject();

            // Check if this object has any included relations
            bool hasIncludes = false;
            for (const QString& relationName : included_relations) {
                if (obj.contains(relationName) && obj[relationName].isArray()) {
                    hasIncludes = true;
                    break;
                }
            }

            if (!hasIncludes) {
                // No includes, add as-is
                flatArray.append(obj);
                continue;
            }

            // Get the first included relation
            QString firstRelation = included_relations[0];
            if (obj.contains(firstRelation) && obj[firstRelation].isArray()) {
                QJsonArray relatedArray = obj[firstRelation].toArray();

                if (relatedArray.isEmpty()) {
                    // No related records, add base object without the array field
                    QJsonObject cleanObj = obj;
                    cleanObj.remove(firstRelation);
                    flatArray.append(cleanObj);
                } else {
                    // For each related record, create a row combining base + related data
                    for (const auto& relatedVal : relatedArray) {
                        QJsonObject relatedObj = relatedVal.toObject();
                        QJsonObject mergedObj = obj;

                        // Remove the array field
                        mergedObj.remove(firstRelation);

                        // Add related fields to the row
                        for (const QString& key : relatedObj.keys()) {
                            mergedObj.insert(key, relatedObj[key]);
                        }

                        flatArray.append(mergedObj);
                    }
                }
            } else {
                flatArray.append(obj);
            }
        }

        return flatArray;
    }

    // Print included relations in detailed format
    void PrintIncludedRelations(const QJsonArray& jsonArray)
    {
        qDebug() << "\n========== INCLUDED RELATIONS ==========\n";

        for (int i = 0; i < jsonArray.size(); ++i) {
            QJsonObject obj = jsonArray[i].toObject();

            // Print entity identifier
            QString identifier = QString("Person (id: %1, name: %2 %3)")
                                     .arg(obj["id"].toVariant().toString(),
                                          obj["first_name"].toString(),
                                          obj["last_name"].toString());

            qDebug() << "\n" << identifier;

            // Print included relations
            for (const QString& relationName : included_relations) {
                if (obj.contains(relationName)) {
                    QJsonArray relatedArray = obj[relationName].toArray();

                    if (!relatedArray.isEmpty()) {
                        qDebug() << "  ├─" << relationName << ":";

                        for (int j = 0; j < relatedArray.size(); ++j) {
                            QJsonObject related = relatedArray[j].toObject();
                            QString prefix = (j == relatedArray.size() - 1) ? "    └─" : "    ├─";

                            // Print related record details
                            QStringList details;
                            for (const QString& key : related.keys()) {
                                details << QString("%1: %2")
                                .arg(key, related[key].toVariant().toString());
                            }

                            qDebug().noquote() << prefix << "[" << j << "] "
                                               << details.join(", ");
                        }
                    } else {
                        qDebug() << "  ├─" << relationName << ": (no related records)";
                    }
                }
            }
        }

        qDebug() << "\n========================================\n";
    }


    void PrintJsonWithIncludes(const QJsonArray& jsonArray)
    {
        qDebug().noquote() << "[";

        for (int i = 0; i < jsonArray.size(); ++i) {
            QJsonObject obj = jsonArray[i].toObject();

            // Separate base fields from includes
            QJsonObject baseObj;
            QMap<QString, QJsonArray> includeData;

            for (const QString& key : obj.keys()) {
                if (included_relations.contains(key)) {
                    // This is an include relation
                    if (obj[key].isArray()) {
                        includeData[key] = obj[key].toArray();
                    }
                } else {
                    // This is a base field
                    baseObj.insert(key, obj[key]);
                }
            }

            // Sort base object keys
            QJsonObject sortedBase;
            QStringList keys = baseObj.keys();
            std::sort(keys.begin(), keys.end());
            for (const QString& key : keys) {
                sortedBase.insert(key, baseObj[key]);
            }

            // Print opening brace
            qDebug().noquote() << "  {";

            // Print base fields
            QStringList baseKeys = sortedBase.keys();
            for (int j = 0; j < baseKeys.size(); ++j) {
                QString key = baseKeys[j];
                QJsonValue val = sortedBase[key];
                QString comma = (j < baseKeys.size() - 1 || !includeData.isEmpty()) ? "," : "";

                if (val.isString()) {
                    qDebug().noquote() << QString("    \"%1\": \"%2\"%3").arg(key, val.toString(), comma);
                } else if (val.isDouble()) {
                    qDebug().noquote() << QString("    \"%1\": %2%3").arg(key, QString::number(val.toDouble()), comma);
                } else if (val.isBool()) {
                    qDebug().noquote() << QString("    \"%1\": %2%3").arg(key, val.toBool() ? "true" : "false", comma);
                } else if (val.isNull()) {
                    qDebug().noquote() << QString("    \"%1\": null%2").arg(key, comma);
                } else {
                    qDebug().noquote() << QString("    \"%1\": %2%3").arg(key, QString::number(val.toDouble()), comma);
                }
            }

            // Print included relations
            QStringList includeKeys = includeData.keys();
            for (int j = 0; j < includeKeys.size(); ++j) {
                QString relName = includeKeys[j];
                QJsonArray relArray = includeData[relName];
                QString comma = (j < includeKeys.size() - 1) ? "," : "";

                qDebug().noquote() << QString("    \"%1\": [").arg(relName);

                for (int k = 0; k < relArray.size(); ++k) {
                    QJsonObject relObj = relArray[k].toObject();
                    QString relComma = (k < relArray.size() - 1) ? "," : "";

                    // Sort related object keys
                    QJsonObject sortedRel;
                    QStringList relObjKeys = relObj.keys();
                    std::sort(relObjKeys.begin(), relObjKeys.end());
                    for (const QString& key : relObjKeys) {
                        sortedRel.insert(key, relObj[key]);
                    }

                    QJsonDocument relDoc(sortedRel);
                    QString relJson = relDoc.toJson(QJsonDocument::Compact);
                    qDebug().noquote() << QString("      %1%2").arg(relJson, relComma);
                }

                qDebug().noquote() << QString("    ]%1").arg(comma);
            }

            // Print closing brace
            QString rowComma = (i < jsonArray.size() - 1) ? "," : "";
            qDebug().noquote() << QString("  }%1").arg(rowComma);
        }

        qDebug().noquote() << "]";
    }

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


    // Load related data for entities
    void LoadRelatedData(QList<Entity>& entities)
    {
        if (!repository) return;

        for (const QString& relationName : included_relations) {
            LoadRelation(entities, relationName);
        }
    }

    // Load a specific relationship
    void LoadRelation(QList<Entity>& entities, const QString& relationName)
    {
        if (!repository) return;

        // Get relation definition from repository
        const Q1Table& table = repository->GetTable();

        // Find matching relation
        for (const Q1Relation& relation : table.relations) {
            if (relation.top_table == relationName) {
                LoadRelationData(entities, relation);
                break;
            }
        }
    }

    // Execute query to load related data
    void LoadRelationData(QList<Entity>& entities, const Q1Relation& relation)
    {
        if (entities.isEmpty() || !repository) return;

        // Collect all foreign key values from entities
        QStringList fkValues;
        for (const Entity& entity : entities) {
            // Get foreign key value from entity
            QString fkValue = GetForeignKeyValue(entity, relation.foreign_key);
            if (!fkValue.isEmpty() && !fkValues.contains(fkValue)) {
                fkValues.append(fkValue);
            }
        }

        if (fkValues.isEmpty()) return;

        // Build query to load related data
        // Use reference_key (usually "id") instead of top_table_primary_key
        QString query = QString("SELECT * FROM \"%1\" WHERE \"%2\" IN (%3)")
                            .arg(relation.top_table,
                                 relation.reference_key,  // Changed from top_table_primary_key
                                 fkValues.join(", "));

        qDebug() << "Eager Loading Query:" << query;

        // Execute query and store results
        QList<QJsonObject> relatedData = repository->ExecuteRelationQuery(query);

        // Store related data map for later use
        relation_cache[relation.top_table] = relatedData;
    }

    // Get foreign key value from entity
    QString GetForeignKeyValue(const Entity& entity, const QString& fkColumn)
    {
        const QMap<QString, typename Q1Entity<Entity>::PropertyInfo>& propMap =
            repository->GetPropertyMap();

        auto it = propMap.find(fkColumn);
        if (it != propMap.end()) {
            const typename Q1Entity<Entity>::PropertyInfo& info = it.value();
            const char* memberPtr = reinterpret_cast<const char*>(&entity) + info.offset;

            // Convert to string based on type
            switch (info.type) {
            case INTEGER:
            case SMALLINT:
            case BIGINT:
                return QString::number(*reinterpret_cast<const int*>(memberPtr));
            case VARCHAR:
            case TEXT:
            case CHAR:
                return *reinterpret_cast<const QString*>(memberPtr);
            default:
                return QString();
            }
        }

        return QString();
    }

    // Append related data to JSON array
    QJsonArray AppendRelatedDataToJson(const QJsonArray& originalArray)
    {
        QJsonArray result = originalArray;

        for (int i = 0; i < result.size(); ++i) {
            QJsonObject obj = result[i].toObject();

            // Add related data for each relation
            for (const auto& relationName : included_relations) {
                if (relation_cache.contains(relationName)) {
                    QList<QJsonObject> relatedData = relation_cache[relationName];

                    // Get the relation definition to find the foreign key
                    const Q1Table& table = repository->GetTable();
                    const Q1Relation* matchingRelation = nullptr;

                    for (const Q1Relation& rel : table.relations) {
                        if (rel.top_table == relationName) {
                            matchingRelation = &rel;
                            break;
                        }
                    }

                    if (!matchingRelation) continue;

                    // Get the foreign key value from current entity
                    QString fkValue = obj[matchingRelation->foreign_key].toVariant().toString();

                    // Find matching related records by reference key
                    QJsonArray relatedArray;
                    for (const QJsonObject& related : relatedData) {
                        QString refValue = related[matchingRelation->reference_key].toVariant().toString();

                        // Match by reference key
                        if (fkValue == refValue) {
                            relatedArray.append(related);
                        }
                    }

                    // Add the matched related data
                    obj.insert(relationName, relatedArray);
                }
            }

            result[i] = obj;
        }

        return result;
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
    QStringList included_relations;
    QMap<QString, QList<QJsonObject>> relation_cache;
};
