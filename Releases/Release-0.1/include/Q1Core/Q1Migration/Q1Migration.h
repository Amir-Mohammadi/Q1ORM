#ifndef Q1MIGRATION_H
#define Q1MIGRATION_H

#include <QStringList>
#include <QSqlQuery>

#include "Q1Core/Q1Context/Q1Connection.h"
#include "Q1Core/Q1Entity/Q1Column.h"
#include "Q1Core/Q1Entity/Q1Table.h"
#include "Q1Core/Q1Migration/Q1MigrationQuery.h"

#include "Q1ORM_global.h"

class Q1ORM_EXPORT Q1Migration
{
public:
    Q1Migration(Q1Connection &connection);

    QStringList GetDatabases();
    QList<Q1Column> GetColumns(QString table_name);

    bool AddDatabase(QString database_name);
    bool AddTable(Q1Table q1table);
    bool AddColumn(QString table_name, Q1Column &column);

    bool DropTable(QString table_name);
    bool DropColumn(QString table_name, QString column_name);
    bool DropColumnNullable(QString table_name, QString column_name);
    bool DropColumnDefault(QString table_name, QString column_name);

    bool SetColumnNullable(QString table_name, QString column_name);
    bool setColumnDefault(QString table_name, QString column_name, QString default_value);

    bool UpdateColumnSize(QString table_name, QString column_name, int size);

    bool HasNullData(QString table_name, QString column_name);

private:
    Q1Connection connection;
    Q1MigrationQuery translator;
};

#endif // Q1MIGRATION_H
