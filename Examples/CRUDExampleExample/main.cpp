#include <Q1Core/Q1Context/Q1Context.h>
#include <QCoreApplication>
#include <Q1ORM.h>
#include "Person.h"

class Crud
{
public:
    Crud(Q1Connection* conn) : repo(conn) {}

    auto GetList(const QString& where = "", const QString& order = "")
    {
        auto query = repo.Select();
        if (!where.isEmpty()) query.Where(where);
        if (!order.isEmpty()) query.OrderBy(order);
        return query.ToList();
    }

    auto GetByJson(const QString& where = "", const QString& order = "")
    {
        auto query = repo.Select();
        if (!where.isEmpty()) query.Where(where);
        if (!order.isEmpty()) query.OrderBy(order);
        return query.ToJson();
    }

    bool Insert(Person& person)
    {
        return repo.Insert(person);
    }

    bool Update(Person& person, const QString& where_clause)
    {
        return repo.Update(person, where_clause);
    }

    bool Delete(const QString& clause)
    {
        return repo.Delete(clause);
    }

private:
    Person repo;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Q1Connection conn(Q1Driver::POSTGRE_SQL, "localhost", "testDb", "postgres", "123", 5432);

    Crud crud(&conn);
    Person user(&conn);

    // INSERT
    user.first_name = "John";
    user.last_name = "Doe";
    user.age = 30;
    user.address_id = 1;
    if (crud.Insert(user)) {
        qDebug() << "Insert successful";
    }

    // GET ALL with table
    qDebug() << "\n--- All Records ---";
    auto list = crud.GetList();

    // GET JSON
    qDebug() << "\n--- JSON Output ---";
    auto json = crud.GetByJson();
    qDebug().noquote() << json;

    // UPDATE
    qDebug() << "\n--- After Update ---";
    user.first_name = "Jane";
    user.last_name = "Smith";
    user.age = 25;
    if (crud.Update(user, "id = 1")) {
        qDebug() << "Update successful";
        auto updated = crud.GetList();
    }

    // DELETE
    qDebug() << "\n--- After Delete ---";
    if (crud.Delete("id = 1")) {
        qDebug() << "Delete successful";
        auto remaining = crud.GetList();
    }

    conn.Disconnect();
    return a.exec();
}
