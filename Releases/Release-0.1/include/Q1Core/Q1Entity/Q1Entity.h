#ifndef Q1ENTITY_H
#define Q1ENTITY_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QList>
#include <QSqlRecord>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "../../Q1Core/Q1Context/Q1Connection.h"
#include "../../Q1Core/Q1Entity/Q1Table.h"
#include "../../Q1Core/Q1Entity/Q1Column.h"
#include "../../Q1Core/Q1Query/Q1Query.h"

template <typename> class Q1Entity;

// --- Traits to detect static methods ---
template <typename T, typename = void>
struct has_configure_method : std::false_type {};

template <typename T>
struct has_configure_method<T, std::void_t<
                                   decltype(T::ConfigureEntity(std::declval<Q1Entity<T>&>()))
                                   >> : std::true_type {};

template <typename T, typename = void>
struct has_createrelations_method : std::false_type {};

template <typename T>
struct has_createrelations_method<T, std::void_t<
                                         decltype(T::CreateRelations(std::declval<Q1Entity<T>&>()))
                                         >> : std::true_type {};


template <class Entity>
class Q1Entity : public Entity
{
public:
    Q1Entity(Q1Connection* connectionPtr)
        : connection(connectionPtr)
    {

        // Auto-configure entity if static method exists
        if constexpr (has_configure_method<Entity>::value)
        {
            Entity::ConfigureEntity(*this);
        }

        // Auto-create relations if static method exists
        if constexpr (has_createrelations_method<Entity>::value)
        {
            Entity::CreateRelations(*this);
        }

    }



/* ############################################################################### */
/* ********************************* Property ************************************ */
/* ############################################################################### */


    template<typename Member>
    void Property(Member& member, const QString& name, bool nullable = false,
                  bool primary_key = false, const QString& default_value = QString())
    {
        // Skip duplicate registration
        if(property_map.contains(name))
            return;

        QString type = typeid(Member).name();
        Q1ColumnDataType column_type = Q1Column::GetVariableType(type);
        int size = DetermineSize<Member>(column_type);

        QString effective_default = default_value;
        if(primary_key && default_value.isEmpty())
            effective_default = "GENERATED ALWAYS AS IDENTITY";

        table.columns.append(Q1Column(name, column_type, size, nullable, primary_key, effective_default));

        // **Store offset relative to the DTO object**
        PropertyInfo info;
        info.name = name;

        // Compute offset from DTO part, not Q1Entity
        info.offset = reinterpret_cast<char*>(&member) - reinterpret_cast<char*>(static_cast<Entity*>(this));
        info.type = column_type;
        property_map[name] = info;
    }






/* ############################################################################### */
/* ********************************* Relation ************************************ */
/* ############################################################################### */

    // Define relationship between tables
    Q1Relation Relations(const QString& base_table, const QString& top_table,
                         Q1RelationType type, const QString& foreign_key,
                         const QString& top_table_primary_key)
    {


        Q1Relation relation(base_table, top_table, type, foreign_key, top_table_primary_key);
        table.relations.append(relation);
        return relation;
    }


/* ############################################################################### */
/* ************************************ Setter *********************************** */
/* ############################################################################### */


    // Set table name
    void ToTableName(const QString& table_name)
    {
        table.table_name = table_name;
    }


/* ############################################################################### */
/* ********************************* Getter ************************************** */
/* ############################################################################### */

    // Get table definition
    Q1Table GetTable() const
    {
        return table;
    }

    //Get Relation
    Q1Relation GetRelation() const
    {
        return relation;
    }

    // Get last error message
    QString GetLastError() const
    {
        return last_error;
    }


    const QList<Q1Column>& GetTableColumns() const
    {
        return table.columns;
    }



    const QJsonArray& GetLastJson() const
    {
        return lastJson;
    }



/* ############################################################################### */
/* ************************ Crud Opertation ************************************** */
/* ############################################################################### */


    bool CheckTableSchema()
    {
        if (!connection || !connection->Connect())
        {
            qDebug() << "Cannot connect to database";
            return false;
        }

        QString query = QString(
                            "SELECT column_name, column_default, is_nullable, is_identity "
                            "FROM information_schema.columns "
                            "WHERE table_name = '%1' "
                            "ORDER BY ordinal_position"
                            ).arg(table.table_name);

        QSqlQuery sql_query(connection->database);
        if (!sql_query.exec(query))
        {
            qDebug() << "Schema check failed:" << sql_query.lastError().text();
            connection->Disconnect();
            return false;
        }

        qDebug() << "=== Table Schema for" << table.table_name << "===";
        while (sql_query.next())
        {
            QString col_name = sql_query.value(0).toString();
            QString col_default = sql_query.value(1).toString();
            QString is_nullable = sql_query.value(2).toString();
            QString is_identity = sql_query.value(3).toString();

            qDebug() << "Column:" << col_name
                     << "| Default:" << col_default
                     << "| Nullable:" << is_nullable
                     << "| Identity:" << is_identity;
        }
        qDebug() << "================================";

        connection->Disconnect();
        return true;
    }


/* ************************ Insert Opertation ************************************** */


    bool Insert(Entity& entity)
    {
        if (!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return false;
        }

        QStringList columns;
        QStringList placeholders;
        QString pk_column_name;
        bool has_auto_pk = false;

        qDebug() << "\n=== INSERT DEBUG INFO ===";
        qDebug() << "Table:" << table.table_name;
        qDebug() << "Columns in entity definition:";

        // Collect columns for INSERT and detect auto-increment PK
        for (const Q1Column& col : table.columns)
        {
            qDebug() << "  -" << col.name
                     << "| PK:" << col.primary_key
                     << "| Default:" << col.default_value;

            if (col.primary_key)
            {
                pk_column_name = col.name;

                // Check if this PK is auto-generated
                bool is_auto = false;

                if (col.default_value.isEmpty())
                {
                    is_auto = true;
                }
                else if (col.default_value.contains("IDENTITY", Qt::CaseInsensitive) ||
                         col.default_value.contains("SERIAL", Qt::CaseInsensitive) ||
                         col.default_value.contains("nextval", Qt::CaseInsensitive) ||
                         col.default_value.contains("GENERATED", Qt::CaseInsensitive))
                {
                    is_auto = true;
                }

                if (is_auto)
                {
                    has_auto_pk = true;
                    qDebug() << "  --> Detected as auto-increment, will skip in INSERT";
                    continue;
                }
            }

            columns.append("\"" + col.name + "\"");
            placeholders.append("?");
        }

        qDebug() << "Has auto PK:" << has_auto_pk;
        qDebug() << "PK column name:" << pk_column_name;
        qDebug() << "Columns to insert:" << columns.join(", ");

        if (columns.isEmpty())
        {
            last_error = "No columns to insert";
            connection->Disconnect();
            return false;
        }

        // Build INSERT query
        QString queryStr;
        if (has_auto_pk && !pk_column_name.isEmpty())
        {
            queryStr = QString("INSERT INTO \"%1\" (%2) VALUES (%3) RETURNING \"%4\"")
            .arg(table.table_name, columns.join(", "), placeholders.join(", "), pk_column_name);
        }
        else
        {
            queryStr = QString("INSERT INTO \"%1\" (%2) VALUES (%3)")
            .arg(table.table_name, columns.join(", "), placeholders.join(", "));
        }

        QSqlQuery sql_query(connection->database);
        if (!sql_query.prepare(queryStr))
        {
            last_error = sql_query.lastError().text();
            qDebug() << "Query preparation failed:" << last_error;
            connection->Disconnect();
            return false;
        }

        qDebug() << "Prepared query:" << queryStr;
        qDebug() << "\nBinding values:";

        // Bind values for non-auto-increment columns
        int bindIndex = 0;
        for (const Q1Column& col : table.columns)
        {
            // Skip auto-generated primary key
            if (col.primary_key)
            {
                bool is_auto = false;

                if (col.default_value.isEmpty())
                {
                    is_auto = true;
                }
                else if (col.default_value.contains("IDENTITY", Qt::CaseInsensitive) ||
                         col.default_value.contains("SERIAL", Qt::CaseInsensitive) ||
                         col.default_value.contains("nextval", Qt::CaseInsensitive) ||
                         col.default_value.contains("GENERATED", Qt::CaseInsensitive))
                {
                    is_auto = true;
                }

                if (is_auto)
                {
                    continue;
                }
            }

            auto it = property_map.find(col.name);
            QVariant value;

            if (it != property_map.end())
            {
                const PropertyInfo& info = it.value();
                const char* memberPtr = reinterpret_cast<const char*>(&entity) + info.offset;

                switch (col.type)
                {
                case INTEGER: case SMALLINT: case BIGINT:
                    value = *reinterpret_cast<const int*>(memberPtr);
                    break;
                case REAL:
                    value = *reinterpret_cast<const float*>(memberPtr);
                    break;
                case DOUBLE_PRECISION:
                    value = *reinterpret_cast<const double*>(memberPtr);
                    break;
                case BOOLEAN:
                    value = *reinterpret_cast<const bool*>(memberPtr);
                    break;
                case CHAR: case TEXT: case VARCHAR:
                    value = *reinterpret_cast<const QString*>(memberPtr);
                    break;
                case DATE:
                    value = *reinterpret_cast<const QDate*>(memberPtr);
                    break;
                case TIMESTAMP:
                    value = *reinterpret_cast<const QDateTime*>(memberPtr);
                    break;
                default:
                    value = QVariant();
                }
            }
            else
            {
                value = QVariant();
            }

            qDebug() << "  [" << bindIndex << "]" << col.name << "=" << value;
            sql_query.addBindValue(value);
            bindIndex++;
        }

        qDebug() << "\nExecuting query...";

        // Execute INSERT
        if (!sql_query.exec())
        {
            last_error = sql_query.lastError().text();
            qDebug() << "❌ Insert FAILED:" << last_error;
            qDebug() << "Query was:" << queryStr;
            connection->Disconnect();
            return false;
        }

        // Retrieve auto-generated PK
        if (has_auto_pk)
        {
            if (sql_query.next())
            {
                QVariant new_id = sql_query.value(0);
                qDebug() << "✓ Retrieved generated PK:" << new_id;

                auto it = property_map.find(pk_column_name);
                if (it != property_map.end())
                {
                    const PropertyInfo& info = it.value();
                    char* memberPtr = reinterpret_cast<char*>(&entity) + info.offset;

                    Q1Column* pk_col = nullptr;
                    for (Q1Column& col : table.columns)
                    {
                        if (col.name == pk_column_name)
                        {
                            pk_col = &col;
                            break;
                        }
                    }

                    if (pk_col)
                    {
                        switch (pk_col->type)
                        {
                        case INTEGER:
                            *reinterpret_cast<int*>(memberPtr) = new_id.toInt();
                            break;
                        case SMALLINT:
                            *reinterpret_cast<short*>(memberPtr) = static_cast<short>(new_id.toInt());
                            break;
                        case BIGINT:
                            *reinterpret_cast<long long*>(memberPtr) = new_id.toLongLong();
                            break;
                        default:
                            *reinterpret_cast<int*>(memberPtr) = new_id.toInt();
                        }
                        qDebug() << "✓ Set entity." << pk_column_name << "=" << new_id;
                    }
                }
            }
            else
            {
                qDebug() << "⚠ Warning: Expected RETURNING value but got none";
            }
        }

        qDebug() << "✓ Insert successful!";
        qDebug() << "========================\n";
        connection->Disconnect();
        return true;
    }


/* ************************ Update Opertation ************************************** */

    // Update entity in database
    bool Update(Entity& entity, const QString& where_clause)
    {
        if (!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return false;
        }

        QStringList set_clauses;
        QList<QVariant> values;

        qDebug() << "\n=== UPDATE DEBUG INFO ===";
        qDebug() << "Table:" << table.table_name;
        qDebug() << "WHERE clause:" << where_clause;
        qDebug() << "Columns to update:";

        for (const Q1Column& col : table.columns)
        {
            if (col.primary_key)
            {
                qDebug() << "  -" << col.name << "(skipped - primary key)";
                continue; // Don't update primary key
            }

            set_clauses.append("\"" + col.name + "\" = ?");
            QVariant value;
            auto it = property_map.find(col.name);

            if (it != property_map.end())
            {
                const PropertyInfo& info = it.value();
                const char* memberPtr = reinterpret_cast<const char*>(&entity) + info.offset;

                switch (col.type)
                {
                case INTEGER:
                case SMALLINT:
                case BIGINT:
                    value = *reinterpret_cast<const int*>(memberPtr);
                    break;
                case REAL:
                    value = *reinterpret_cast<const float*>(memberPtr);
                    break;
                case DOUBLE_PRECISION:
                    value = *reinterpret_cast<const double*>(memberPtr);
                    break;
                case BOOLEAN:
                    value = *reinterpret_cast<const bool*>(memberPtr);
                    break;
                case CHAR:
                case TEXT:
                case VARCHAR:
                    value = *reinterpret_cast<const QString*>(memberPtr);
                    break;
                case DATE:
                    value = *reinterpret_cast<const QDate*>(memberPtr);
                    break;
                case TIMESTAMP:
                    value = *reinterpret_cast<const QDateTime*>(memberPtr);
                    break;
                default:
                    value = QVariant();
                }
            }
            else
            {
                value = QVariant();
            }

            qDebug() << "  -" << col.name << "=" << value;
            values.append(value);
        }

        if (set_clauses.isEmpty())
        {
            last_error = "No columns to update";
            qDebug() << "❌ Update FAILED: No columns to update";
            connection->Disconnect();
            return false;
        }

        if (where_clause.isEmpty())
        {
            last_error = "UPDATE without WHERE clause is dangerous and not allowed";
            qDebug() << "❌ Update FAILED: WHERE clause is required";
            connection->Disconnect();
            return false;
        }

        QString query = QString("UPDATE \"%1\" SET %2 WHERE %3")
                            .arg(table.table_name, set_clauses.join(", "), where_clause);

        qDebug() << "Query:" << query;
        qDebug() << "Binding" << values.size() << "values...";

        QSqlQuery sql_query(connection->database);
        if (!sql_query.prepare(query))
        {
            last_error = sql_query.lastError().text();
            qDebug() << "❌ Query preparation failed:" << last_error;
            connection->Disconnect();
            return false;
        }

        // Bind values in the order of set_clauses
        for (int i = 0; i < values.size(); ++i)
        {
            sql_query.bindValue(i, values[i]);
        }

        qDebug() << "Executing update...";

        bool success = sql_query.exec();
        if (!success)
        {
            last_error = sql_query.lastError().text();
            qDebug() << "❌ Update FAILED:" << last_error;
            qDebug() << "Query was:" << query;
            connection->Disconnect();
            return false;
        }

        int rows_affected = sql_query.numRowsAffected();
        qDebug() << "✓ Update successful!";
        qDebug() << "Rows affected:" << rows_affected;

        if (rows_affected == 0)
        {
            qDebug() << "⚠ Warning: No rows were updated. Check your WHERE clause.";
        }

        qDebug() << "========================\n";
        connection->Disconnect();
        return true;
    }



    bool UpdateById(Entity& entity, int id)
    {
        QString pk_name;

        // Find the primary key column name
        for (const Q1Column& col : table.columns)
        {
            if (col.primary_key)
            {
                pk_name = col.name;
                break;
            }
        }

        if (pk_name.isEmpty())
        {
            last_error = "No primary key defined for this table";
            qDebug() << "❌ UpdateById FAILED: No primary key found";
            return false;
        }

        QString where_clause = QString("\"%1\" = %2").arg(pk_name).arg(id);
        return Update(entity, where_clause);
    }


/* ************************ Delete Opertation ************************************** */


    // Delete entity from database
    bool Delete(const QString& where_clause)
    {
        if (!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return false;
        }

        qDebug() << "\n=== DELETE DEBUG INFO ===";
        qDebug() << "Table:" << table.table_name;
        qDebug() << "WHERE clause:" << where_clause;

        if (where_clause.isEmpty())
        {
            last_error = "DELETE without WHERE clause is dangerous and not allowed";
            qDebug() << "❌ Delete FAILED: WHERE clause is required for safety";
            connection->Disconnect();
            return false;
        }

        QString query = QString("DELETE FROM \"%1\" WHERE %2")
                            .arg(table.table_name, where_clause);

        qDebug() << "Query:" << query;
        qDebug() << "Executing delete...";

        QSqlQuery sql_query(connection->database);
        bool success = sql_query.exec(query);

        if (!success)
        {
            last_error = sql_query.lastError().text();
            qDebug() << "❌ Delete FAILED:" << last_error;
            qDebug() << "Query was:" << query;
            connection->Disconnect();
            return false;
        }

        int rows_affected = sql_query.numRowsAffected();
        qDebug() << "✓ Delete successful!";
        qDebug() << "Rows affected:" << rows_affected;

        if (rows_affected == 0)
        {
            qDebug() << "⚠ Warning: No rows were deleted. Check your WHERE clause.";
        }

        qDebug() << "========================\n";
        connection->Disconnect();
        return true;
    }


    bool DeleteById(int id)
    {
        QString pk_name;

        // Find the primary key column name
        for (const Q1Column& col : table.columns)
        {
            if (col.primary_key)
            {
                pk_name = col.name;
                break;
            }
        }

        if (pk_name.isEmpty())
        {
            last_error = "No primary key defined for this table";
            qDebug() << "❌ DeleteById FAILED: No primary key found";
            return false;
        }

        QString where_clause = QString("\"%1\" = %2").arg(pk_name).arg(id);
        return Delete(where_clause);
    }



/* ************************ Select Opertation ************************************** */


    Q1Query<Entity> Select()
    {
        return Q1Query<Entity>(this);
    }

    Q1Query<Entity> SelectJoin(const QStringList& columns)
    {
        Q1Query<Entity> query(this);
        query.SetColumns(columns);
        return query;
    }

    QList<Entity> SelectAll()
    {
        return SelectExec(QString(), QString(), -1);
    }

    // Select entities from database
    QList<Entity> SelectExec(const QString& where_clause = QString(),
                         const QString& order_by = QString(),
                         int limit = -1,
                         const QString& joins = QString(),
                         const QStringList& columns = QStringList(),
                         const QString& group_by = QString(),
                         const QString& having_clause = QString())
    {
        lastJson = QJsonArray(); // Clear previous JSON
        QList<Entity> results;

        if (!connection || !connection->Connect()) {
            last_error = "Database connection failed";
            return results;
        }

        // Build SQL query
        QString query;

        // SELECT clause
        if (!columns.isEmpty()) {
            query = "SELECT " + columns.join(", ");
        } else {
            query = QString("SELECT * FROM \"%1\"").arg(table.table_name);
        }

        // FROM clause (only if columns are specified)
        if (!columns.isEmpty()) {
            query += QString(" FROM \"%1\"").arg(table.table_name);
        }

        // JOIN clause
        if (!joins.isEmpty()) {
            query += joins;
        }

        // WHERE clause
        if (!where_clause.isEmpty()) {
            query += " WHERE " + where_clause;
        }

        // GROUP BY clause
        if (!group_by.isEmpty()) {
            query += " GROUP BY " + group_by;
        }

        // HAVING clause
        if (!having_clause.isEmpty()) {
            query += " HAVING " + having_clause;
        }

        // ORDER BY clause
        if (!order_by.isEmpty()) {
            query += " ORDER BY " + order_by;
        }

        // LIMIT clause
        if (limit > 0) {
            query += " LIMIT " + QString::number(limit);
        }

        qDebug() << "SQL Query:" << query;

        QSqlQuery sql_query(connection->database);
        if (!sql_query.exec(query)) {
            last_error = sql_query.lastError().text();
            qDebug() << "Select failed:" << last_error;
            connection->Disconnect();
            return results;
        }

        QSqlRecord rec = sql_query.record();
        QStringList allColumnNames;

        // Get all column names from result set
        for (int i = 0; i < rec.count(); ++i) {
            allColumnNames << rec.fieldName(i);
        }

        while (sql_query.next()) {
            Entity entity;

            // Populate entity members from table.columns
            for (const Q1Column& col : table.columns) {
                auto it = property_map.find(col.name);
                if (it == property_map.end()) continue;

                const PropertyInfo& info = it.value();
                char* memberPtr = reinterpret_cast<char*>(&entity) + info.offset;

                // Get column index (works for simple select and joins)
                int colIndex = rec.indexOf(col.name);
                if (colIndex < 0) continue;

                QVariant val = sql_query.value(colIndex);

                if (val.isNull()) {
                    switch (col.type) {
                    case INTEGER:
                    case SMALLINT:
                    case BIGINT:
                        *reinterpret_cast<int*>(memberPtr) = 0;
                        break;
                    case REAL:
                        *reinterpret_cast<float*>(memberPtr) = 0.0f;
                        break;
                    case DOUBLE_PRECISION:
                        *reinterpret_cast<double*>(memberPtr) = 0.0;
                        break;
                    case BOOLEAN:
                        *reinterpret_cast<bool*>(memberPtr) = false;
                        break;
                    case CHAR:
                    case TEXT:
                    case VARCHAR:
                        *reinterpret_cast<QString*>(memberPtr) = QString();
                        break;
                    case DATE:
                        *reinterpret_cast<QDate*>(memberPtr) = QDate();
                        break;
                    case TIMESTAMP:
                        *reinterpret_cast<QDateTime*>(memberPtr) = QDateTime();
                        break;
                    default:
                        break;
                    }
                } else {
                    switch (col.type) {
                    case INTEGER:
                        *reinterpret_cast<int*>(memberPtr) = val.toInt();
                        break;
                    case SMALLINT:
                        *reinterpret_cast<short*>(memberPtr) = static_cast<short>(val.toInt());
                        break;
                    case BIGINT:
                        *reinterpret_cast<long long*>(memberPtr) = val.toLongLong();
                        break;
                    case REAL:
                        *reinterpret_cast<float*>(memberPtr) = static_cast<float>(val.toDouble());
                        break;
                    case DOUBLE_PRECISION:
                        *reinterpret_cast<double*>(memberPtr) = val.toDouble();
                        break;
                    case BOOLEAN:
                        *reinterpret_cast<bool*>(memberPtr) = val.toBool();
                        break;
                    case CHAR:
                    case TEXT:
                    case VARCHAR:
                        *reinterpret_cast<QString*>(memberPtr) = val.toString();
                        break;
                    case DATE:
                        *reinterpret_cast<QDate*>(memberPtr) = val.toDate();
                        break;
                    case TIMESTAMP:
                        *reinterpret_cast<QDateTime*>(memberPtr) = val.toDateTime();
                        break;
                    default:
                        break;
                    }
                }
            }

            // Convert ALL result columns to JSON (including joined columns)
            QJsonObject obj;
            for (const QString& colName : allColumnNames) {
                QVariant val = sql_query.value(rec.indexOf(colName));

                if (val.isNull()) {
                    obj.insert(colName, QJsonValue::Null);
                } else if (val.type() == QVariant::Int) {
                    obj.insert(colName, val.toInt());
                } else if (val.type() == QVariant::Double) {
                    obj.insert(colName, val.toDouble());
                } else if (val.type() == QVariant::Bool) {
                    obj.insert(colName, val.toBool());
                } else if (val.type() == QVariant::Date) {
                    obj.insert(colName, val.toDate().toString(Qt::ISODate));
                } else if (val.type() == QVariant::DateTime) {
                    obj.insert(colName, val.toDateTime().toString(Qt::ISODate));
                } else {
                    obj.insert(colName, val.toString());
                }
            }

            lastJson.append(obj);
            results.append(entity);
        }

        connection->Disconnect();
        return results;
    }




private:
    template<typename T>
    int DetermineSize(Q1ColumnDataType type)
    {
        if (type == VARCHAR || type == CHAR)
            return 255; // Default string size
        return 0; // Numeric types don't need size
    }

public:
    QJsonObject EntityToJson(const Entity &entity) const
    {
        QJsonObject obj; // Create an actual QJsonObject instance

        for (const Q1Column &col : table.columns)
        {
            auto it = property_map.find(col.name);
            if (it == property_map.end())
            {
                obj.insert(col.name, QJsonValue());
                continue;
            }

            const PropertyInfo &info = it.value();
            const char* memberPtr = reinterpret_cast<const char*>(&entity) + info.offset;

            switch (col.type)
            {
            case INTEGER: case SMALLINT: case BIGINT:
            {
                int v = *reinterpret_cast<const int*>(memberPtr);
                obj.insert(col.name, v);
                break;
            }
            case REAL:
            {
                float v = *reinterpret_cast<const float*>(memberPtr);
                obj.insert(col.name, double(v));
                break;
            }
            case DOUBLE_PRECISION:
            {
                double v = *reinterpret_cast<const double*>(memberPtr);
                obj.insert(col.name, v);
                break;
            }
            case BOOLEAN:
            {
                bool v = *reinterpret_cast<const bool*>(memberPtr);
                obj.insert(col.name, v);
                break;
            }
            case CHAR: case TEXT: case VARCHAR:
            {
                const QString &s = *reinterpret_cast<const QString*>(memberPtr);
                obj.insert(col.name, s);
                break;
            }
            case DATE:
            {
                const QDate &d = *reinterpret_cast<const QDate*>(memberPtr);
                if (d.isValid()) obj.insert(col.name, d.toString(Qt::ISODate));
                else obj.insert(col.name, QJsonValue());
                break;
            }
            case TIMESTAMP:
            {
                const QDateTime &dt = *reinterpret_cast<const QDateTime*>(memberPtr);
                if (dt.isValid()) obj.insert(col.name, dt.toString(Qt::ISODate));
                else obj.insert(col.name, QJsonValue());
                break;
            }
            default:
                obj.insert(col.name, QJsonValue());
                break;
            }
        }

        return obj; // Return the QJsonObject by value
    }



    QVariant ExecuteScalar(const QString& sql)
    {
        if(!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return QVariant();
        }

        QSqlQuery query(connection->database);
        if(!query.exec(sql))
        {
            last_error = query.lastError().text();
            qDebug() << "ExecuteScalar failed: " << last_error;
            connection->Disconnect();
            return QVariant();
        }

        QVariant result;
        if(query.next())
        {
            result = query.value(0);
        }


        connection->Disconnect();
        return result;
    }

public:
    struct PropertyInfo
    {
        QString name;
        ptrdiff_t offset;
        Q1ColumnDataType type;
    };

    const QMap<QString, PropertyInfo>& GetPropertyMap() const
    {
        return property_map;
    }

private:
    Q1Table table;
    Q1Relation relation;
    Q1Connection* connection;
    QMap<QString, PropertyInfo> property_map;
    QString last_error;
    QJsonArray lastJson;
};




#endif // Q1ENTITY_H
