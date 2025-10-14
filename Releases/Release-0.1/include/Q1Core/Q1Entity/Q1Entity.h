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

    // Set table name
    void ToTableName(const QString& table_name)
    {
        table.table_name = table_name;
    }

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

    // Define relationship between tables
    Q1Relation Relations(const QString& base_table, const QString& top_table,
                         Q1RelationType type, const QString& foreign_key,
                         const QString& top_table_primary_key)
    {


        Q1Relation relation(base_table, top_table, type, foreign_key, top_table_primary_key);
        table.relations.append(relation);
        return relation;
    }


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

    // CRUD Operations

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

        // Collect columns for INSERT and detect auto-increment PK
        for (const Q1Column& col : table.columns)
        {
            if (col.primary_key)
            {
                pk_column_name = col.name;
                // Skip auto-increment primary key (no default value means auto-increment)
                if (col.default_value.isEmpty())
                {
                    has_auto_pk = true;
                    continue;
                }
            }

            columns.append("\"" + col.name + "\"");
            placeholders.append("?");
        }

        if (columns.isEmpty())
        {
            last_error = "No columns to insert";
            connection->Disconnect();
            return false;
        }

        // Build INSERT query with RETURNING clause for auto-increment PK
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

        // Bind values for non-auto-increment columns
        for (const Q1Column& col : table.columns)
        {
            // Skip auto-increment PK
            if (col.primary_key && col.default_value.isEmpty() && col.default_value == "GENERATED ALWAYS AS IDENTITY")
                continue;

            auto it = property_map.find(col.name);
            QVariant value;

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

                qDebug() << "Binding" << col.name << "=" << value;
            }
            else
            {
                qDebug() << "Warning: Column" << col.name << "not found in property_map";
            }

            sql_query.addBindValue(value);
        }

        if (!sql_query.exec())
        {
            last_error = sql_query.lastError().text();
            qDebug() << "Insert failed:" << last_error;
            qDebug() << "Query:" << queryStr;
            connection->Disconnect();
            return false;
        }

        // Retrieve the auto-generated PK value and update entity
        if (has_auto_pk && sql_query.next())
        {
            QVariant new_id = sql_query.value(0);
            qDebug() << "Generated PK:" << new_id;

            // Update entity with new ID
            auto it = property_map.find(pk_column_name);
            if (it != property_map.end())
            {
                const PropertyInfo& info = it.value();
                char* memberPtr = reinterpret_cast<char*>(&entity) + info.offset;

                // Cast to appropriate integer type based on column type
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
                }
            }
        }

        qDebug() << "Insert successful!";
        connection->Disconnect();
        return true;
    }

    // Update entity in database
    bool Update(const QString& where_clause)
    {
        if (!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return false;
        }

        QStringList set_clauses;

        for (const Q1Column& col : table.columns)
        {
            if (col.primary_key)
                continue; // Don't update primary key

            set_clauses.append("\"" + col.name + "\" = ?");
        }

        QString query = QString("UPDATE \"%1\" SET %2")
                            .arg(table.table_name, set_clauses.join(", "));

        if (!where_clause.isEmpty())
        {
            query += " WHERE " + where_clause;
        }

        QSqlQuery sql_query(connection->database);
        sql_query.prepare(query);

        // Note: Binding values needs proper implementation

        bool success = sql_query.exec();
        if (!success)
        {
            last_error = sql_query.lastError().text();
            qDebug() << "Update failed:" << last_error;
        }

        connection->Disconnect();
        return success;
    }

    // Delete entity from database
    bool Delete(const QString& where_clause)
    {
        if (!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return false;
        }

        QString query = QString("DELETE FROM \"%1\"").arg(table.table_name);

        if (!where_clause.isEmpty())
        {
            query += " WHERE " + where_clause;
        }
        else
        {
            last_error = "DELETE without WHERE clause is dangerous and not allowed";
            connection->Disconnect();
            return false;
        }

        QSqlQuery sql_query(connection->database);
        bool success = sql_query.exec(query);

        if (!success)
        {
            last_error = sql_query.lastError().text();
            qDebug() << "Delete failed:" << last_error;
        }

        connection->Disconnect();
        qDebug() << "[Q1Orm]: Delete Successfully ";
        return success;
    }




    Q1Query<Entity> Select()
    {
        return Q1Query<Entity>(this);
    }

    QList<Entity> SelectAll()
    {
        return SelectExec(QString(), QString(), -1);
    }

    // Select entities from database
    QList<Entity> SelectExec(const QString& where_clause = QString(),
                         const QString& order_by = QString(),
                         int limit = -1)
    {
        lastJson = QJsonArray(); // Clear previous JSON

        QList<Entity> results;

        if (!connection || !connection->Connect()) {
            last_error = "Database connection failed";
            return results;
        }

        // Build SQL query
        QString query = QString("SELECT * FROM \"%1\"").arg(table.table_name);
        if (!where_clause.isEmpty()) query += " WHERE " + where_clause;
        if (!order_by.isEmpty()) query += " ORDER BY " + order_by;
        if (limit > 0) query += " LIMIT " + QString::number(limit);

        QSqlQuery sql_query(connection->database);
        if (!sql_query.exec(query)) {
            last_error = sql_query.lastError().text();
            qDebug() << "Select failed:" << last_error;
            connection->Disconnect();
            return results;
        }

        QSqlRecord rec = sql_query.record();

        while (sql_query.next()) {
            Entity entity;

            // Populate entity members
            for (const Q1Column& col : table.columns) {
                auto it = property_map.find(col.name);
                if (it == property_map.end()) continue;

                const PropertyInfo& info = it.value();
                char* memberPtr = reinterpret_cast<char*>(&entity) + info.offset;
                QVariant val = sql_query.value(rec.indexOf(col.name));

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

            // Convert entity to JSON
            QJsonObject obj;
            for (const Q1Column& col : table.columns) {
                auto it = property_map.find(col.name);
                if (it == property_map.end()) continue;

                const PropertyInfo& info = it.value();
                const char* memberPtr = reinterpret_cast<const char*>(&entity) + info.offset;

                switch (col.type) {
                case INTEGER:
                case SMALLINT:
                case BIGINT:
                    obj.insert(col.name, *reinterpret_cast<const int*>(memberPtr));
                    break;

                case REAL:
                    obj.insert(col.name, static_cast<double>(*reinterpret_cast<const float*>(memberPtr)));
                    break;

                case DOUBLE_PRECISION:
                    obj.insert(col.name, *reinterpret_cast<const double*>(memberPtr));
                    break;

                case BOOLEAN:
                    obj.insert(col.name, *reinterpret_cast<const bool*>(memberPtr));
                    break;

                case CHAR:
                case TEXT:
                case VARCHAR:
                    obj.insert(col.name, *reinterpret_cast<const QString*>(memberPtr));
                    break;

                case DATE: {
                    const QDate& d = *reinterpret_cast<const QDate*>(memberPtr);
                    obj.insert(col.name, d.isValid() ? d.toString(Qt::ISODate) : QString());
                    break;
                }

                case TIMESTAMP: {
                    const QDateTime& dt = *reinterpret_cast<const QDateTime*>(memberPtr);
                    obj.insert(col.name, dt.isValid() ? dt.toString(Qt::ISODate) : QString());
                    break;
                }

                default:
                    obj.insert(col.name, QString());
                    break;
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

    struct PropertyInfo
    {
        QString name;
        ptrdiff_t offset;
        Q1ColumnDataType type;
    };


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

private:
    Q1Table table;
    Q1Relation relation;
    Q1Connection* connection;
    QMap<QString, PropertyInfo> property_map;
    QString last_error;
    QJsonArray lastJson;
};




#endif // Q1ENTITY_H
