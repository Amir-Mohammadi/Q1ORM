#include "Q1Migration.h"
#include <QSqlError>
#include <QSqlIndex>
#include <QDebug>
#include <QRegularExpression>

Q1Migration::Q1Migration(Q1Connection &connection)
    : connection(connection)
{
    if (connection.GetDriver() == Q1Driver::POSTGRE_SQL)
        translator = Q1MigrationQuery(DatabaseType::PostgreSQL);
    else if (connection.GetDriver() == Q1Driver::SQLSERVER)
        translator = Q1MigrationQuery(DatabaseType::SQLServer);
    else if (connection.GetDriver() == Q1Driver::MYSQL)
        translator = Q1MigrationQuery(DatabaseType::MySQL);
    else if (connection.GetDriver() == Q1Driver::SQLITE)
        translator = Q1MigrationQuery(DatabaseType::SQLite);
}

QStringList Q1Migration::GetDatabases()
{
    QStringList databases;

    if (connection.IsSqlite())
    {
        databases.append(connection.GetDatabaseName());
        return databases;
    }

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
    m_lastError.clear();
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
    m_lastError.clear();
    QList<Q1Column> columns;

    if (!connection.Connect())
    {
        m_lastError = "Cannot connect: " + connection.ErrorMessage();
        return columns;
    }

    const QSqlIndex primary = connection.database.primaryIndex(table_name);
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
            column.primary_key = primary.indexOf(column.name) >= 0;
            column.nullable = !column.primary_key && (sql.value("is_nullable").toString() == "YES");
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
    if (connection.IsSqlite())
    {
        Q_UNUSED(database_name);
        return connection.Connect();
    }

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

    if (!connection.BeginTransaction())
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
        connection.RollbackTransaction();
        connection.Disconnect();
        return false;
    }

    if (!connection.CommitTransaction())
    {
        m_lastError = "Failed to commit: " + db.lastError().text();
        qWarning() << m_lastError;
        connection.RollbackTransaction();
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

    if (!connection.BeginTransaction())
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
        connection.RollbackTransaction();
        connection.Disconnect();
        return false;
    }

    if (!connection.CommitTransaction())
    {
        m_lastError = "Failed to commit: " + db.lastError().text();
        connection.RollbackTransaction();
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
    QList<QString> lowerTables;
    for (const QString &t : existingTables) {
        lowerTables.append(t.toLower());
    }

    if (!lowerTables.contains(relation.base_table.toLower()) ||
        !lowerTables.contains(relation.top_table.toLower()))
    {
        m_lastError = "Tables missing: " + relation.base_table + " or " + relation.top_table;
        qWarning() << "[Warning]" << m_lastError;
        return false;
    }
    translator.SetDatabase(connection.database);
    QString sql = translator.AddRelationSQL(relation);
    if (sql.isEmpty())
    {
        m_lastError = "No SQL generated for relation";
        return false;
    }

    if (!connection.BeginTransaction())
    {
        m_lastError = "Failed to start transaction: " + connection.database.lastError().text();
        return false;
    }

    QSqlQuery q(connection.database);
    QStringList statements = sql.split(';', Qt::SkipEmptyParts);

    QRegularExpression addConstraintRx("\\badd\\s+constraint\\s+((\"[^\"]+?\")|([A-Za-z0-9_]+))",
                                       QRegularExpression::CaseInsensitiveOption);

    for (QString stmt : statements)
    {
        stmt = stmt.trimmed();
        if (stmt.isEmpty()) continue;

        QString constraintName;

        if (stmt.toLower().contains(" add constraint "))
        {
            QRegularExpressionMatch match = addConstraintRx.match(stmt);
            if (match.hasMatch())
            {
                constraintName = match.captured(1);

                if (constraintName.startsWith('"') && constraintName.endsWith('"') && constraintName.size() >= 2)
                {
                    constraintName = constraintName.mid(1, constraintName.size() - 2);
                }

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
            connection.RollbackTransaction();
            m_lastError = err;
            qWarning() << "[Error] Failed to execute relation SQL:" << err << "\nQuery:" << stmt;
            return false;
        }
    }

    if (!connection.CommitTransaction())
    {
        m_lastError = connection.ErrorMessage();
        qWarning() << "[Warning] Failed to commit transaction:" << m_lastError;
        connection.RollbackTransaction();
        return false;
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
    if (query.isEmpty())
    {
        m_lastError = "This column alteration is unsupported by the selected database driver.";
        connection.Disconnect();
        return false;
    }

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
    if (query.isEmpty())
    {
        m_lastError = "This column alteration is unsupported by the selected database driver.";
        connection.Disconnect();
        return false;
    }

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
    if (query.isEmpty())
    {
        m_lastError = "This column alteration is unsupported by the selected database driver.";
        connection.Disconnect();
        return false;
    }

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
    if (query.isEmpty())
    {
        m_lastError = "This column alteration is unsupported by the selected database driver.";
        connection.Disconnect();
        return false;
    }

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
    if (query.isEmpty())
    {
        connection.Disconnect();
        return true;
    }

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
    m_lastError.clear();
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
    m_lastError.clear();
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


bool Q1Migration::EnsureIndexes(const Q1Table& table)
{
    m_lastError.clear();
    if (!connection.Connect()) { m_lastError = connection.ErrorMessage(); return false; }
    bool success = true;
    for (const Q1Table::Index& index : table.GetIndexes()) {
        QStringList columns;
        for (const QString& name : index.columns) {
            if (!table.HasColumn(name)) {
                m_lastError = QString("Index %1 references unknown column %2").arg(index.name, name);
                success = false;
                break;
            }
            columns << connection.QuoteIdentifier(name);
        }
        if (!success) break;
        QSqlQuery check(connection.database);
        QString checkSql;
        if (connection.IsPostgreSql())
            checkSql = "SELECT 1 FROM pg_indexes WHERE schemaname = current_schema() AND tablename = ? AND indexname = ?";
        else if (connection.IsSqlServer())
            checkSql = "SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(?) AND name = ?";
        else if (connection.IsMySql())
            checkSql = "SELECT 1 FROM information_schema.statistics WHERE table_schema = DATABASE() AND table_name = ? AND index_name = ?";
        else
            checkSql = "SELECT 1 FROM sqlite_master WHERE type = 'index' AND tbl_name = ? AND name = ?";
        if (!check.prepare(checkSql)) { m_lastError = check.lastError().text(); success = false; break; }
        check.addBindValue(table.GetName());
        check.addBindValue(index.name);
        if (!check.exec()) { m_lastError = check.lastError().text(); success = false; break; }
        const bool exists = check.next();
        check.finish();
        if (exists) continue;
        const QString sql = QString("CREATE %1INDEX %2 ON %3 (%4)")
            .arg(index.unique ? "UNIQUE " : "", connection.QuoteIdentifier(index.name),
                 connection.QuoteIdentifier(table.GetName()), columns.join(", "));
        QSqlQuery create(connection.database);
        if (!create.exec(sql)) { m_lastError = create.lastError().text(); success = false; break; }
    }
    connection.Disconnect();
    return success;
}

Q1Migration::~Q1Migration()
{
    RollbackSchemaUpdate();
}

bool Q1Migration::BeginSchemaUpdate()
{
    m_lastError.clear();
    if (m_schemaUpdateActive) {
        m_lastError = "A schema update is already active.";
        return false;
    }
    if (!connection.IsOpen()) {
        m_lastError = "Schema setup requires an open database connection.";
        return false;
    }
    if (connection.InTransaction()) {
        m_lastError = "Initialize must run outside an application transaction.";
        return false;
    }
    if (connection.IsSqlite()) {
        QSqlQuery settings(connection.database);
        if (!settings.exec("PRAGMA foreign_keys") || !settings.next()) {
            m_lastError = settings.lastError().text();
            return false;
        }
        m_restoreForeignKeys = settings.value(0).toInt() != 0;
        settings.finish();
        if (m_restoreForeignKeys && !settings.exec("PRAGMA foreign_keys = OFF")) {
            m_lastError = settings.lastError().text();
            m_restoreForeignKeys = false;
            return false;
        }
    }
    if (!connection.IsMySql() && !connection.BeginTransaction()) {
        m_lastError = connection.ErrorMessage();
        RestoreForeignKeys();
        return false;
    }
    m_schemaUpdateActive = true;
    return true;
}

bool Q1Migration::CommitSchemaUpdate()
{
    if (!m_schemaUpdateActive) {
        m_lastError = "No schema update is active.";
        return false;
    }
    if (connection.IsMySql()) {
        // MySQL DDL commits implicitly, so no outer schema transaction is used.
        m_schemaUpdateActive = false;
        return true;
    }
    if (connection.IsSqlite()) {
        QSqlQuery check(connection.database);
        if (!check.exec("PRAGMA foreign_key_check")) {
            m_lastError = check.lastError().text();
            return false;
        }
        if (check.next()) {
            m_lastError = QString("Foreign-key validation failed for table '%1', row %2.")
                .arg(check.value(0).toString(), check.value(1).toString());
            return false;
        }
    }
    const bool committed = connection.CommitTransaction();
    m_schemaUpdateActive = false;
    if (!committed) m_lastError = connection.ErrorMessage();
    RestoreForeignKeys();
    return committed;
}

void Q1Migration::RollbackSchemaUpdate()
{
    if (!m_schemaUpdateActive) return;
    if (!connection.IsMySql()) connection.RollbackTransaction();
    m_schemaUpdateActive = false;
    RestoreForeignKeys();
}

void Q1Migration::RestoreForeignKeys()
{
    if (!m_restoreForeignKeys) return;
    QSqlQuery restore(connection.database);
    restore.exec("PRAGMA foreign_keys = ON");
    m_restoreForeignKeys = false;
}
