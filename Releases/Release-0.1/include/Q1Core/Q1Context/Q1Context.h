#ifndef Q1CONTEXT_H
#define Q1CONTEXT_H

#include <QList>
#include <QString>
#include <QDebug>
#include <typeindex>

#include "../Q1Entity/Q1Table.h"
#include "../Q1Entity/Q1Column.h"
#include "../Q1Entity/Q1Relation.h"
#include "../Q1Entity/Q1EntityBase.h"
#include "../Q1Entity/Q1Entity.h"
#include "Q1Connection.h"
#include "Q1Core/Q1Migration/Q1Migration.h"

class Q1ModelBuilder;

class Q1ORM_EXPORT Q1Context
{
public:
    Q1Context() = default;
    virtual ~Q1Context();

    void RegisterEntity(Q1EntityBase *entity)
    {
        if (!entity)
            return;
        entity->SetConnection(connection);
        if (!entities.contains(entity))
            entities.append(entity);
    }

    template <typename TEntity>
    Q1Entity<TEntity> *ResolveEntity() const
    {
        const std::type_index wanted(typeid(TEntity));
        for (Q1EntityBase *e : entities) {
            if (e && e->EntityType() == wanted)
                return static_cast<Q1Entity<TEntity> *>(e);
        }
        return nullptr;
    }

    bool Initialize();

    QString GetLastError() const
    {
        if (!model_error.isEmpty())
            return model_error;
        if (query && !query->ErrorMessage().isEmpty())
            return query->ErrorMessage();
        if (connection)
            return connection->ErrorMessage();
        return QString();
    }

protected:
    virtual void OnModelCreating(Q1ModelBuilder& builder) = 0;

    void SetConnection(Q1Connection *conn, bool takeOwnership = false)
    {
        connection = conn;
        owns_connection = takeOwnership;
        for (auto *entity : entities)
            entity->SetConnection(connection);
    }

    bool InitialDatabase();
    bool InitialTables(const QList<Q1Table *> &model);
    bool InitialColumns(const QList<Q1Table *> &model);
    void CompareColumn(const QString &table_name, Q1Column &dbColumn, Q1Column &declColumn);
    bool InitialRelations(const QList<Q1Relation> &relations);

protected:
    Q1Connection *connection = nullptr;
    Q1Migration *query = nullptr;
    QList<Q1EntityBase *> entities;
    QString database_name;
    QString model_error;

    bool check_columns = true;
    bool allow_destructive_migrations = false;
    bool owns_connection = false;
};

#endif // Q1CONTEXT_H
