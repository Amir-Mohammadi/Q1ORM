#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "IRepository.h"
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <Q1ORM.h>
#include <QDebug>

template<typename T>
class Repository : public IRepository<T>
{
public:
    Repository(Q1Entity<T>& entity) : entity(entity) {}

    // --- CRUD ---
    bool Insert(T& obj) override
    {
        return entity.Insert(obj);
    }

    bool Update(T& obj, const QString& where_clause) override
    {
        return entity.Update(obj, where_clause);
    }

    bool Delete(int id) override
    {
        return entity.DeleteById(id);
    }

    T SelectById(int id) override
    {
        Q1Query<T> query(&entity);
        QList<T> list = query.Where(QString("id=%1").arg(id)).ToList();
        if (!list.isEmpty()) return list.first();
        return T{};
    }

    QList<T> SelectAll() override
    {
        Q1Query<T> query(&entity);
        return query.ShowList();
    }

    QList<T> SelectJoin(const QString& table, const QString& onClause)
    {
        Q1Query<T> query(&entity);
        query.InnerJoin(table, onClause);
        return query.ShowList();
    }

    QList<T> SelectInclude(const QStringList& relations)
    {
        Q1Query<T> query(&entity);
        query.Include(relations);
        return query.ShowList();
    }

private:
    Q1Entity<T>& entity;
};

#endif // REPOSITORY_H
