#include "Q1Migration.h"
#include <QSqlError>
#include <QDebug>

Q1Migration::Q1Migration(Q1Connection &connection)
    : connection(connection)
{
    if (connection.GetDriver() == Q1Driver::POSTGRE_SQL)
        translator = Q1MigrationQuery(DatabaseType::PostgreSQL);
    else if (connection.GetDriver() == Q1Driver::SQLSERVER)
        translator = Q1MigrationQuery(DatabaseType::SQLServer);
}

QStringList Q1Migration::GetDatabases()
{
    QStringList databases;

    if (!connection.RootConnect())
    {
        m_lastError = "Cannot connect to server: " + connection.ErrorMessage();
        return databases;
    }

    QString query = translator.GetDatabasesSQL();
    QSqlQuery sql(connection.root_database);

    if (!sql.exec(query))
    {
        m_lastError = sql.lastError().text();
        qWarning() << "GetDatabases failed:" << m_lastError;
    }
    else
    {
        while (sql.next())
            databases.append(sql.value(0).toString());
    }

    connection.RootDisconnect();
    return databases;
}

QStringList Q1Migration::GetTables()
{
    QStringList tables;

    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return tables;
    }

    tables = connection.database.tables();
    connection.Disconnect();

    return tables;
}

QList<Q1Column> Q1Migration::GetColumns(QString table_name)
{
    QList<Q1Column> columns;

    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return columns;
    }

    QString query = translator.GetColumnsSQL(table_name);
    QSqlQuery sql(connection.database);

    if (!sql.exec(query))
    {
        m_lastError = sql.lastError().text();
        qWarning() << "GetColumns failed:" << m_lastError;
    }
    else
    {
        while (sql.next())
        {
            Q1Column column;
            column.name = sql.value("column_name").toString();
            column.type = Q1Column::GetColumnType(sql.value("data_type").toString());
            column.size = sql.value("character_maximum_length").toInt();
            column.nullable = (sql.value("is_nullable").toString() == "YES");
            const QVariant identityValue = sql.value("is_identity");
            const QString identityText = identityValue.toString();
            const bool isIdentity = identityValue.toBool() ||
                                    identityValue.toInt() != 0 ||
                                    identityText.compare("YES", Qt::CaseInsensitive) == 0 ||
                                    identityText.compare("true", Qt::CaseInsensitive) == 0;
            column.is_identity = isIdentity;
            column.default_value = isIdentity
                                       ? QStringLiteral("GENERATED ALWAYS AS IDENTITY")
                                       : Q1Column::NormalizeDefaultValue(sql.value("column_default").toString());

            columns.append(column);
        }
    }

    connection.Disconnect();
    return columns;
}

bool Q1Migration::AddDatabase(QString database_name)
{
    if (!connection.RootConnect())
    {
        m_lastError = "Cannot connect to server: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.AddDatabaseSQL(database_name);
    QSqlQuery sql(connection.root_database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "AddDatabase failed:" << m_lastError;
    }

    connection.RootDisconnect();
    return success;
}

bool Q1Migration::CreateTableWithColumns(Q1Table& q1table)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QSqlDatabase &db = connection.database;

    if (!db.transaction())
    {
        m_lastError = "Failed to start transaction: " + db.lastError().text();
        qWarning() << m_lastError;
        connection.Disconnect();
        return false;
    }

    QString query = translator.AddTableSQL(q1table);
    QSqlQuery sql(db);

    qDebug() << "CreateTableWithColumns - executing:" << query;

    bool success = sql.exec(query);

    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "CreateTableWithColumns failed:" << m_lastError;
        db.rollback();
        connection.Disconnect();
        return false;
    }

    if (!db.commit())
    {
        m_lastError = "Failed to commit: " + db.lastError().text();
        qWarning() << m_lastError;
        db.rollback();
        connection.Disconnect();
        return false;
    }

    qDebug() << "CreateTableWithColumns - table created successfully:" << q1table.GetName();
    connection.Disconnect();
    return true;
}

bool Q1Migration::AddTable(Q1Table q1table)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.AddTableSQL(q1table);
    QSqlQuery sql(connection.database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "AddTable failed:" << m_lastError;
    }

    connection.Disconnect();
    return success;
}

bool Q1Migration::AddColumn(QString table_name, Q1Column &column)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QSqlDatabase &db = connection.database;

    if (!db.transaction())
    {
        m_lastError = "Failed to start transaction: " + db.lastError().text();
        connection.Disconnect();
        return false;
    }

    QString query = translator.AddColumnSQL(table_name, column);
    QSqlQuery sql(db);

    qDebug() << "AddColumn - executing:" << query;

    bool success = sql.exec(query);

    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "AddColumn failed:" << m_lastError;
        db.rollback();
        connection.Disconnect();
        return false;
    }

    if (!db.commit())
    {
        m_lastError = "Failed to commit: " + db.lastError().text();
        db.rollback();
        connection.Disconnect();
        return false;
    }

    connection.Disconnect();
    return true;
}

bool Q1Migration::AddRelation(const Q1Relation &relation)
{
    if (!relation.IsValid())
    {
        m_lastError = "Invalid relation";
        return false;
    }

    if (!connection.database.isOpen())
    {
        if (!connection.Connect())
        {
            m_lastError = connection.ErrorMessage();
            return false;
        }
    }

    QStringList existingTables = connection.database.tables();
    for (QString &t : existingTables) t = t.toLower();

    if (!existingTables.contains(relation.base_table.toLower()) ||
        !existingTables.contains(relation.top_table.toLower()))
    {
        m_lastError = "Tables missing: " + relation.base_table + " or " + relation.top_table;
        qWarning() << "[Warning]" << m_lastError;
        return false;
    }

    QString sql = translator.AddRelationSQL(relation);
    if (sql.isEmpty())
    {
        m_lastError = "No SQL generated for relation";
        return false;
    }

    QSqlQuery q(connection.database);

    bool startedTx = connection.database.transaction();
    if (!startedTx)
    {
        if (!q.exec("BEGIN"))
        {
            m_lastError = q.lastError().text();
            qWarning() << "Failed to begin transaction:" << m_lastError;
            return false;
        }
        startedTx = true;
    }

    QStringList statements = sql.split(';', Qt::SkipEmptyParts);
    QRegExp addConstraintRx("\\badd\\s+constraint\\s+((\"[^\"]+\")|([A-Za-z0-9_]+))", Qt::CaseInsensitive);
    addConstraintRx.setMinimal(true);

    for (QString stmt : statements)
    {
        stmt = stmt.trimmed();
        if (stmt.isEmpty()) continue;

        QString constraintName;
        if (stmt.toLower().contains(" add constraint "))
        {
            int pos = addConstraintRx.indexIn(stmt);
            if (pos != -1) constraintName = addConstraintRx.cap(1);

            if (!constraintName.isEmpty())
            {
                if (constraintName.startsWith('"') && constraintName.endsWith('"') && constraintName.size() >= 2)
                    constraintName = constraintName.mid(1, constraintName.size() - 2);

                if (ConstraintExists(connection.database, constraintName))
                {
                    qDebug() << "[Info] Skipping existing constraint:" << constraintName;
                    continue;
                }
            }
        }

        if (!q.exec(stmt))
        {
            QString err = q.lastError().text();
            if (startedTx)
            {
                connection.database.rollback();
            }
            m_lastError = err;
            qWarning() << "[Error] Failed to execute relation SQL:" << err << "\nQuery:" << stmt;
            return false;
        }
    }

    if (startedTx)
    {
        if (!connection.database.commit())
        {
            qWarning() << "[Warning] Failed to commit relation transaction:" << connection.database.lastError().text();
            connection.database.rollback();
            return false;
        }
    }

    return true;
}

bool Q1Migration::DropTable(QString table_name)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.DropTableSQL(table_name);
    QSqlQuery sql(connection.database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "DropTable failed:" << m_lastError;
    }

    connection.Disconnect();
    return success;
}

bool Q1Migration::DropColumn(QString table_name, QString column_name)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.DropColumnSQL(table_name, column_name);
    QSqlQuery sql(connection.database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "DropColumn failed:" << m_lastError;
    }

    connection.Disconnect();
    return success;
}

bool Q1Migration::DropColumnNullable(QString table_name, QString column_name)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.DropColumnNullableSQL(table_name, column_name);
    QSqlQuery sql(connection.database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "DropColumnNullable failed:" << m_lastError;
    }

    connection.Disconnect();
    return success;
}

bool Q1Migration::DropColumnDefault(QString table_name, QString column_name)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.DropColumnDefaultSQL(table_name, column_name);
    QSqlQuery sql(connection.database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "DropColumnDefault failed:" << m_lastError;
    }

    connection.Disconnect();
    return success;
}

bool Q1Migration::SetColumnNullable(QString table_name, QString column_name)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.SetColumnNullableSQL(table_name, column_name);
    QSqlQuery sql(connection.database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "SetColumnNullable failed:" << m_lastError;
    }

    connection.Disconnect();
    return success;
}

bool Q1Migration::setColumnDefault(QString table_name, QString column_name, QString default_value)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.SetColumnDefaultSQL(table_name, column_name, default_value);
    QSqlQuery sql(connection.database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "setColumnDefault failed:" << m_lastError;
    }

    connection.Disconnect();
    return success;
}

bool Q1Migration::UpdateColumnSize(QString table_name, QString column_name, int size)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.UpdateColumnSizeSQL(table_name, column_name, size);
    QSqlQuery sql(connection.database);

    bool success = sql.exec(query);
    if (!success)
    {
        m_lastError = sql.lastError().text();
        qWarning() << "UpdateColumnSize failed:" << m_lastError;
    }

    connection.Disconnect();
    return success;
}

bool Q1Migration::HasNullData(QString table_name, QString column_name)
{
    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return false;
    }

    QString query = translator.HasNullDataSQL(table_name, column_name);
    QSqlQuery sql(connection.database);

    bool hasNull = false;
    if (sql.exec(query) && sql.next())
    {
        hasNull = (sql.value(0).toInt() > 0);
    }
    else
    {
        m_lastError = sql.lastError().text();
    }

    connection.Disconnect();
    return hasNull;
}

bool Q1Migration::ConstraintExists(QSqlDatabase &db, const QString &constraint_name)
{
    QString query = translator.ConstraintExistsSQL(constraint_name);
    QSqlQuery sql(db);

    if (!sql.exec(query))
    {
        m_lastError = sql.lastError().text();
        qWarning() << "ConstraintExists query failed:" << m_lastError;
        return false;
    }

    if (sql.next())
    {
        int count = sql.value(0).toInt();
        return (count > 0);
    }

    return false;
}
