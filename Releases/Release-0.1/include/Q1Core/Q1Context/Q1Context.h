#ifndef Q1CONTEXT_H
#define Q1CONTEXT_H

#include <QList>
#include <QString>
#include <QDebug>

#include "../Q1Entity/Q1Table.h"
#include "../Q1Entity/Q1Column.h"
#include "Q1Connection.h"
#include "Q1Core/Q1Migration/Q1Migration.h"

class Q1ORM_EXPORT Q1Context
{
public:
    Q1Context() = default;
    virtual ~Q1Context();

    void Initialize();

protected:
    // Must override in derived class
    virtual void OnConfiguration() = 0;
    virtual QList<Q1Table> OnTablesCreating() = 0;
    virtual QList<Q1Relation> OnTableRelationCreating() = 0;

    void InitialDatabase();
    void InitialTables();
    void InitialColumns(Q1Table &q1table);
    void CompareColumn(const QString &table_name, Q1Column &dbColumn, Q1Column &declColumn);
    void InitialRelations(const QList<Q1Relation> &relations);

protected:
    Q1Connection *connection = nullptr;
    Q1Migration *query = nullptr;

    QString database_name;
    QList<Q1Table> q1tables;
    QStringList tables;

    bool check_columns = true;

};

#endif // Q1CONTEXT_H
