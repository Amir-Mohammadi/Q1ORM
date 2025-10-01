#ifndef Q1ENTITY_H
#define Q1ENTITY_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QDebug>
#include <QList>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <typeinfo>

#include "../../Q1Core/Q1Context/Q1Connection.h"
#include "../../Q1Core/Q1Entity/Q1Table.h"
#include "../../Q1Core/Q1Entity/Q1Column.h"

template <class Entity>
class Q1Entity : public Entity
{
public:
    Q1Entity(Q1Connection* connectionPtr)
        : connection(connectionPtr)
    {
    }

    // Set table name
    void ToTableName(const QString& table_name)
    {
        table.table_name = table_name;
    }

    // Register property (column mapping)
    template<typename Member>
    void Property(Member& member, const QString& name, bool nullable = false,
                  bool primary_key = false, const QString& default_value = QString())
    {
        // Get C++ type name
        QString type = typeid(Member).name();

        // Convert to SQL column type
        Q1ColumnDataType column_type = Q1Column::GetVariableType(type);

        // Determine appropriate size
        int size = DetermineSize<Member>(column_type);

        // Create column definition
        Q1Column column(name, column_type, size, nullable, primary_key, default_value);
        table.columns.append(column);

        // Store property mapping for data binding
        PropertyInfo info;
        info.name = name;
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

    // CRUD Operations

    // Insert entity into database
    bool Insert(const Entity& entity)
    {
        if (!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return false;
        }

        QStringList columns;
        QStringList placeholders;

        for (const Q1Column& col : table.columns)
        {
            if (col.primary_key && col.default_value.isEmpty())
                continue; // skip auto-increment PK

            columns.append("\"" + col.name + "\"");
            placeholders.append("?");
        }

        QString queryStr = QString("INSERT INTO \"%1\" (%2) VALUES (%3)")
                               .arg(table.table_name, columns.join(", "), placeholders.join(", "));

        QSqlQuery sql_query(connection->database);
        sql_query.prepare(queryStr);

        // Bind values from entity members using offsets
        for (const Q1Column& col : table.columns)
        {
            if (col.primary_key && col.default_value.isEmpty())
                continue;

            auto it = property_map.find(col.name);
            if (it != property_map.end())
            {
                const PropertyInfo& info = it.value();
                const char* memberPtr = reinterpret_cast<const char*>(&entity) + info.offset;

                QVariant value;

                switch (info.type)
                {
                case this->INT:    value = *reinterpret_cast<const int*>(memberPtr); break;
                case this->FLOAT:  value = *reinterpret_cast<const float*>(memberPtr); break;
                case this->DOUBLE: value = *reinterpret_cast<const double*>(memberPtr); break;
                case this->STRING: value = *reinterpret_cast<const QString*>(memberPtr); break;
                default: value = QVariant();
                }

                sql_query.addBindValue(value);
            }
            else
            {
                sql_query.addBindValue(QVariant()); // fallback
            }
        }

        bool success = sql_query.exec();
        if (!success)
        {
            last_error = sql_query.lastError().text();
            qDebug() << "Insert failed:" << last_error;
            qDebug() << "Query:" << queryStr;
        }

        connection->Disconnect();
        return success;
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
        return success;
    }

    // Select entities from database
    QList<Entity> Select(const QString& where_clause = QString(),
                         const QString& order_by = QString(),
                         int limit = -1)
    {
        QList<Entity> results;

        if (!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return results;
        }

        QString query = QString("SELECT * FROM \"%1\"").arg(table.table_name);

        if (!where_clause.isEmpty())
        {
            query += " WHERE " + where_clause;
        }

        if (!order_by.isEmpty())
        {
            query += " ORDER BY " + order_by;
        }

        if (limit > 0)
        {
            query += " LIMIT " + QString::number(limit);
        }

        QSqlQuery sql_query(connection->database);

        if (!sql_query.exec(query))
        {
            last_error = sql_query.lastError().text();
            qDebug() << "Select failed:" << last_error;
            connection->Disconnect();
            return results;
        }

        while (sql_query.next())
        {
            Entity entity;
            // Note: Mapping query results to entity properties
            // requires proper reflection or manual implementation
            results.append(entity);
        }

        connection->Disconnect();
        return results;
    }

    // Count records
    int Count(const QString& where_clause = QString())
    {
        if (!connection || !connection->Connect())
        {
            last_error = "Database connection failed";
            return -1;
        }

        QString query = QString("SELECT COUNT(*) FROM \"%1\"").arg(table.table_name);

        if (!where_clause.isEmpty())
        {
            query += " WHERE " + where_clause;
        }

        QSqlQuery sql_query(connection->database);

        if (!sql_query.exec(query))
        {
            last_error = sql_query.lastError().text();
            connection->Disconnect();
            return -1;
        }

        int count = 0;
        if (sql_query.next())
        {
            count = sql_query.value(0).toInt();
        }

        connection->Disconnect();
        return count;
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

private:
    Q1Table table;
    Q1Relation relation;
    Q1Connection* connection;
    QMap<QString, PropertyInfo> property_map;
    QString last_error;
};

#endif // Q1ENTITY_H
