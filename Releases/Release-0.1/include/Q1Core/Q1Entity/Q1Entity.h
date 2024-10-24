#ifndef Q1ENTITY_H
#define Q1ENTITY_H

#include <QString>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>
#include <QList>

#include "Q1Core/Q1Context/Q1Connection.h"
#include "Q1Core/Q1Entity/Q1Table.h"
#include "qsqlerror.h"
#include "qsqlquery.h"
#include <Q1Core/Q1Entity/Q1Column.h>
#include <Q1Core/Q1Migration/Q1MigrationQuery.h>

template <class Entity>
class Q1Entity : public Entity
{
public:
    Q1Entity(Q1Connection *connectionPtr): connection(*connectionPtr) { }

    void ToTableName(QString table_name)
    {
        table.table_name = table_name;
    }

    // template<typename Member>
    // void Property(Member& member, QString name, bool nullable = false, bool primary_key = false, QString default_value = nullptr)
    // {
    //     QRegularExpression regex("\\d");
    //     QString type = typeid(member).name();
    //     type = type.replace(regex, "");

    //     Q1Column column(name,
    //                     Q1Column::GetVariableType(type),
    //                     sizeof(member),
    //                     nullable,
    //                     primary_key,
    //                     default_value);
    //     table.columns.append(column);
    // }

    template<typename Member>
    void Property(Member& member, const QString& name, bool nullable = false, bool primary_key = false, const QString& default_value = QString()) {
        QRegularExpression regex("\\d");
        QString type = typeid(member).name();
        type = type.replace(regex, "");

        Q1Column column(name,
                        Q1Column::GetVariableType(type),
                        sizeof(member),
                        nullable,
                        primary_key,
                        default_value);
        table.columns.append(column);
    }

    Q1Relation Relations(const QString& base_table, const QString& top_table, Q1RelationType type, const QString& foreign_key, const QString& top_table_primary_key) {
        Q1Relation relation;
        relation.base_table = base_table;
        relation.top_table = top_table;
        relation.foreign_key = foreign_key;
        relation.type = type;

        QString query;
        QString constraint_name = "FK_" + base_table + "_" + top_table + "_" + foreign_key;

        if (type == Q1RelationType::ONE_TO_ONE) {
            query = QString("ALTER TABLE \"%1\" "
                            "ADD CONSTRAINT %2 "
                            "UNIQUE (%3), "
                            "ADD FOREIGN KEY (%3) "
                            "REFERENCES \"%4\" (%5);")
                        .arg(base_table, constraint_name, foreign_key, top_table, top_table_primary_key);
        } else if (type == Q1RelationType::ONE_TO_MANY) {
            query = QString("ALTER TABLE \"%1\" "
                            "ADD CONSTRAINT %2 "
                            "FOREIGN KEY (%3) "
                            "REFERENCES \"%4\" (%5);")
                        .arg(base_table, constraint_name, foreign_key, top_table, top_table_primary_key);
        } else if (type == Q1RelationType::MANY_TO_ONE) {
            query = QString("ALTER TABLE \"%1\" "
                            "ADD CONSTRAINT %2 "
                            "FOREIGN KEY (%3) "
                            "REFERENCES \"%4\" (%5);")
                        .arg(base_table, constraint_name, foreign_key, top_table, top_table_primary_key);
        } else if (type == Q1RelationType::MANY_TO_MANY) {
            QString junction_table = base_table + "_" + top_table;
            QString junction_constraint1 = "FK_" + junction_table + "_" + base_table + "_" + foreign_key;
            QString junction_constraint2 = "FK_" + junction_table + "_" + top_table + "_" + top_table_primary_key;

            query = QString("CREATE TABLE IF NOT EXISTS \"%1\" ("
                            "\"%2\" INT NOT NULL, "
                            "\"%3\" INT NOT NULL, "
                            "PRIMARY KEY (\"%2\", \"%3\"), "
                            "CONSTRAINT %4 FOREIGN KEY (\"%2\") REFERENCES \"%5\" (\"%6\"), "
                            "CONSTRAINT %7 FOREIGN KEY (\"%3\") REFERENCES \"%8\" (\"%9\"));")
                        .arg(junction_table, foreign_key, top_table_primary_key, junction_constraint1, base_table, foreign_key,
                             junction_constraint2, top_table, top_table_primary_key);
        }

        qDebug() << query;
        if (!sql_query.exec(query)) {
            qDebug() << "Error executing query:" << sql_query.lastError().text();
            // Handle the error appropriately
        }

        return relation;
    }

    // Q1Relation Relations(QString base_table, QString top_table, Q1RelationType type, QString foreign_key, QString top_table_primary_key)
    // {
    //     Q1Relation relation;

    //     connection.Connect();

    //     relation.base_table = base_table;
    //     relation.top_table = top_table;
    //     relation.foreign_key = foreign_key;
    //     relation.type = type;

    //     QString query;

    //     QString constraint_name = "FK_" + base_table + "_" + top_table + "_" + foreign_key;

    //     if(Q1RelationType::ONE_TO_ONE == type)
    //     {
    //         // query = (

    //         //     "ALTER TABLE \"" + base_table + "\"" +
    //         //         " ADD CONSTRAINT " + constraint_name +
    //         //         " UNIQUE (" + foreign_key + "), " + // unique key for one-to-one relationship
    //         //         " ADD FOREIGN KEY (" + foreign_key +
    //         //         ") REFERENCES \"" + top_table +
    //         //         "\" (" + top_table_primary_key + ");"
    //         //     );
    //         // qDebug() << query;

    //         query = QString("ALTER TABLE \"%1\" "
    //                         "ADD CONSTRAINT %2 "
    //                         "UNIQUE (%3), "
    //                         "ADD FOREIGN KEY (%3) "
    //                         "REFERENCES \"%4\" (%5);").arg(base_table, constraint_name, foreign_key, top_table, top_table_primary_key);;
    //         if (!sql_query.exec(query)) {
    //             qDebug() << "Error executing query:" << sql_query.lastError().text();
    //             // Handle the error appropriately
    //         }

    //         sql_query.exec(query);

    //     }
    //     else if(Q1RelationType::ONE_TO_MANY == type)
    //     {


    //         query = (
    //             "ALTER TABLE \"" + base_table + "\"" +
    //                      " ADD CONSTRAINT " + constraint_name +
    //                      " FOREIGN KEY (" + foreign_key +
    //                      ") REFERENCES \"" + top_table +
    //                      "\" (" + top_table_primary_key + ");"
    //             );

    //         qDebug() << query;
    //         if (!sql_query.exec(query)) {
    //             qDebug() << "Error executing query:" << sql_query.lastError().text();
    //             // Handle the error appropriately
    //         }

    //         sql_query.exec(query);
    //     }
    //     else if(Q1RelationType::MANY_TO_ONE == type)
    //     {
    //         qDebug() << "* -> 1";
    //     }
    //     else if(Q1RelationType::MANY_TO_MANY == type)
    //     {
    //         qDebug() << "* -> *";
    //     }

    //     qDebug() << relation.base_table;
    //     qDebug() << relation.top_table;
    //     qDebug() << relation.foreign_key;
    //     qDebug() << relation.type;

    //     return relation;

    //     connection.Disconnect();
    // }

    Q1Table GetTable()
    {
        return table;
    }

private:
    Q1Table table;
    QSqlQuery sql_query;
    Q1Connection connection;
};

#endif // Q1Builder_H
