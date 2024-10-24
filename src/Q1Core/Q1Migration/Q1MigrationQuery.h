#ifndef Q1MIGRATIONQUERY_H
#define Q1MIGRATIONQUERY_H

#include <QList>
#include <QDebug>

#include "Q1Core/Q1Entity/Q1Column.h"
#include "Q1Core/Q1Entity/Q1Table.h"

class Q1ORM_EXPORT Q1MigrationQuery
{
public:
    Q1MigrationQuery() { }

    QString GetDatabases();
    QString GetColumns(QString table_name);

    QString AddDatabase(QString database_name, QString username);
    QString AddTable(Q1Table &q1table);
    QString AddColumn(QString table_name, Q1Column &column);

    QString DropTable(QString table_name);
    QString DropColumn(QString table_name, QString column_name);
    QString DropColumnNullable(QString table_name, QString column_name);
    QString DropColumnDefault(QString table_name, QString column_name);

    QString SetColumnNullable(QString table_name, QString column_name);
    QString setColumnDefault(QString table_name, QString column_name, QString default_value);

    QString UpdateColumnSize(QString table_name, QString column_name, int size);

    QString HasNulllData(QString table_name, QString column_name);


private:
    QString ColumnProperty(Q1Column &column);
};

#endif // Q1MIGRATIONQUERY_H
