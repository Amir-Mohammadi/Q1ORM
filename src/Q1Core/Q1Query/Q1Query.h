#pragma once
#include <QString>
#include <QList>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

template<typename Entity> class Q1Entity; // forward

template<typename Entity>
class Q1Query
{
public:
    explicit Q1Query(Q1Entity<Entity>* repo = nullptr)
        : repository(repo), limit_val(-1) {}


    Q1Query& Where(const QString& clause)
    {
        where_clause = clause;
        return *this;
    }

    Q1Query& OrderBy(const QString& clause)
    {
        order_by = clause;
        return *this;
    }

    Q1Query& Limit(int l)
    {
        limit_val = l;
        return *this;
    }


    Q1Query& OrderByAsc(const QString& column)
    {
        if(!order_by.isEmpty())
            order_by += ", ";
        order_by += QString("%1 ASC").arg(column);
        return *this;
    }

    Q1Query& OrderByDesc(const QString& column)
    {
        if(!order_by.isEmpty())
            order_by += ", ";
        order_by += QString("%1 DESC").arg(column);
        return *this;
    }


    operator QList<Entity>()
    {
        return ToList();
    }

    QList<Entity> ToList()
    {
        if (!repository) return {};
        results = repository->SelectExec(where_clause, order_by, limit_val);
        return results;
    }

    // Return JSON for the same result
    QByteArray ToJson()
    {
        if (results.isEmpty()) ToList(); // ensure results are loaded

        QJsonArray array;
        for (const auto& entity : results)
        {
            array.append(repository->EntityToJson(entity));
        }

        // Return pretty JSON as QByteArray
        QJsonDocument doc(array);
        return doc.toJson(QJsonDocument::Indented);
    }


private:
    Q1Entity<Entity>* repository = nullptr;
    QString where_clause;
    QString order_by;
    int limit_val;
    QList<Entity> results;
};
