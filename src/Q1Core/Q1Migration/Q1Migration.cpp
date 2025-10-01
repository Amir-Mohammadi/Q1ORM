#include "Q1Migration.h"

Q1Migration::Q1Migration(Q1Connection &connection) : connection(connection) { }


/* ############################################################################### */
/* ************************************ Get ************************************** */
/* ############################################################################### */

QStringList Q1Migration::GetDatabases()
{
    QStringList databases = {};
    QString database_name = "";

    QString query = translator.GetDatabases();

    connection.RootConnect();

    QSqlQuery sql_query(connection.root);
    bool is_executed = sql_query.exec(query);

    if(!is_executed)
    {
        connection.RootDisconnect();
        return databases;
    }

    sql_query.first();

    for(int i = 0; i < sql_query.numRowsAffected(); i++)
    {
        database_name = sql_query.value("datname").toString();
        databases.append(database_name);

        sql_query.next();
    }

    connection.RootDisconnect();
    return databases;
}

QList<Q1Column> Q1Migration::GetColumns(QString table_name)
{
    QList<Q1Column> columns;

    QString query = translator.GetColumns(table_name);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    if(!is_executed)
    {
        connection.Disconnect();
        return columns;
    }

    sql_query.first();

    for(int i = 0; i < sql_query.numRowsAffected(); i++)
    {
        QString name = sql_query.value("column_name").toString(),
                default_value = sql_query.value("column_default").toString();

        bool is_nullable = sql_query.value("is_nullable").toString() == "YES",
             is_primary_key = sql_query.value("constraint_name").toString().contains("pkey");

        int size = sql_query.value("character_maximum_length").toString().toInt();
        Q1ColumnDataType type = Q1Column::GetColumnType(sql_query.value("data_type").toString());

        Q1Column column(name, type, size, is_nullable, is_primary_key, default_value);
        columns.append(column);

        sql_query.next();
    }

    connection.Disconnect();

    return columns;
}


/* ############################################################################### */
/* ************************************ Add ************************************** */
/* ############################################################################### */

bool Q1Migration::AddDatabase(QString database_name)
{
    QString query = translator.AddDatabase(database_name, connection.GetUsername());

    connection.RootConnect();

    QSqlQuery sql_query(connection.root);
    bool is_executed = sql_query.exec(query);

    connection.RootDisconnect();

    return is_executed;
}

bool Q1Migration::AddTable(Q1Table q1table)
{
    QString query = translator.AddTable(q1table);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;
}

bool Q1Migration::AddColumn(QString table_name, Q1Column &column)
{
    QString query = translator.AddColumn(table_name, column);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;
}


/* ############################################################################### */
/* *********************************** Drop ************************************** */
/* ############################################################################### */

bool Q1Migration::DropTable(QString table_name)
{
    QString query = translator.DropTable(table_name);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;
}

bool Q1Migration::DropColumn(QString table_name, QString column_name)
{
    QString query = translator.DropColumn(table_name, column_name);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;
}

bool Q1Migration::DropColumnNullable(QString table_name, QString column_name)
{
    QString query = translator.DropColumnNullable(table_name, column_name);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;
}

bool Q1Migration::DropColumnDefault(QString table_name, QString column_name)
{
    QString query = translator.DropColumnDefault(table_name, column_name);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;
}


/* ############################################################################### */
/* ************************************ Set ************************************** */
/* ############################################################################### */

bool Q1Migration::SetColumnNullable(QString table_name, QString column_name)
{
    QString query = translator.SetColumnNullable(table_name, column_name);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;
}

bool Q1Migration::setColumnDefault(QString table_name, QString column_name, QString default_value)
{
    QString query = translator.setColumnDefault(table_name, column_name,default_value);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;
}


/* ############################################################################### */
/* ********************************* Update ************************************** */
/* ############################################################################### */

bool Q1Migration::UpdateColumnSize(QString table_name, QString column_name, int size)
{
    QString query = translator.UpdateColumnSize(table_name, column_name, size);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    connection.Disconnect();

    return is_executed;

}


/* ############################################################################### */
/* ************************************ Has ************************************** */
/* ############################################################################### */

bool Q1Migration::HasNullData(QString table_name, QString column_name)
{
    QString query = translator.HasNulllData(table_name, column_name);

    connection.Connect();

    QSqlQuery sql_query(connection.database);
    bool is_executed = sql_query.exec(query);

    if(!is_executed)
    {
        connection.Disconnect();
        return true;
    }

    bool is_null = (sql_query.numRowsAffected() != 0);

    connection.Disconnect();

    return is_null;
}


/* ############################################################################### */
/********************************* Relation ************************************** */
/* ############################################################################### */

bool Q1Migration::AddRelation(const Q1Relation &relation)
{
    if (!relation.IsValid())
    {
        m_lastError = "Invalid relation";
        return false;
    }

    // Ensure DB open
    if (!connection.database.isOpen())
    {
        if(!connection.Connect())
        {
            m_lastError = connection.ErrorMessage();
            return false;
        }
    }

    // Ensure both tables exist
    QStringList existingTables = connection.database.tables();
    if (!existingTables.contains(relation.base_table) || !existingTables.contains(relation.top_table))
    {
        m_lastError = QString("Table missing: %1 or %2").arg(relation.base_table, relation.top_table);
        return false;
    }

    QString sql = translator.AddRelationSQL(relation);
    if (sql.isEmpty())
    {
        m_lastError = "No SQL generated for relation";
        return false;
    }

    QSqlQuery q(connection.database);
    // Start transaction
    if (!q.exec("BEGIN"))
    {
        m_lastError = q.lastError().text();
        qWarning() << "Failed to begin transaction:" << m_lastError;
        return false;
    }

    // Split by ';' — handle multiple statements
    QStringList statements = sql.split(';', Qt::SkipEmptyParts);
    for (QString stmt : statements)
    {
        stmt = stmt.trimmed();
        if (stmt.isEmpty()) continue;

        // If it's a UNIQUE statement, check if unique constraint name exists
        // We assume translator uses UQ_<base>_<col> naming for unique.
        // Parse constraint name for UNIQUE if needed (simple approach):
        QString lower = stmt.toLower();
        if (lower.startsWith("alter table") && lower.contains(" add constraint "))
        {
            // extract constraint token between "ADD CONSTRAINT " and next space
            int pos = lower.indexOf(" add constraint ");
            int start = pos + QString(" add constraint ").length();
            // get token until space
            QString rest = stmt.mid(start).trimmed();
            int end = rest.indexOf(' ');
            QString constraintName = (end == -1) ? rest : rest.left(end);
            // If constraint exists skip this statement
            if (ConstraintExists(connection.database, constraintName))
            {
                qDebug() << "Skipping existing constraint:" << constraintName;
                continue;
            }
        }

        if (!q.exec(stmt))
        {
            QString err = q.lastError().text();
            q.exec("ROLLBACK");
            m_lastError = err;
            qWarning() << "Failed to execute relation SQL:" << err << "Query was:" << stmt;
            return false;
        }
    }

    if (!q.exec("COMMIT"))
    {
        QString err = q.lastError().text();
        q.exec("ROLLBACK");
        m_lastError = err;
        qWarning() << "Failed to commit relation transaction:" << err;
        return false;
    }

    return true;
}




/* ############################################################################### */
/********************************* ConstraintExists ****************************** */
/* ############################################################################### */

bool Q1Migration::ConstraintExists(QSqlDatabase &db, const QString &constraint_name)
{
    if (!db.isOpen())
        return false;

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT 1
        FROM information_schema.table_constraints
        WHERE constraint_name = :constraint
    )");
    query.bindValue(":constraint", constraint_name);
    if (!query.exec())
    {
        qWarning() << "ConstraintExists query failed:" << query.lastError().text();
        return false;
    }

    return query.next(); // if a row exists, constraint exists
}

