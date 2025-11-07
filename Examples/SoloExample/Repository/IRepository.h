#ifndef IREPOSITORY_H
#define IREPOSITORY_H

#include <QList>
#include <QString>

template <typename T>
class IRepository
{
public:
    virtual ~IRepository() = default;

    virtual bool Insert(T& entity) = 0;
    virtual bool Update(T& entity, const QString& where_clause) = 0;
    virtual bool Delete(int id) = 0;
    virtual T SelectById(int id) = 0;
    virtual QList<T> SelectAll() = 0;
};

#endif // IREPOSITORY_H
