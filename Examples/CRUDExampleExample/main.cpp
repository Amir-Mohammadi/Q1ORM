
#include <Q1Core/Q1Context/Q1Context.h>
#include <QCoreApplication>
#include <Q1ORM.h>

#include "Person.h"



class Crud
{
public:

    Crud(Q1Connection* conn) : connection(conn), repo(conn) {}

    QList<PersonDto> GetList(const QString& where = "", const QString& order = "id ASC")
    {
        auto query = repo.Select();
        if (!where.isEmpty()) query.Where(where);
        if (!order.isEmpty()) query.OrderBy(order);
        return query.ToList();
    }

    auto GetByJson()
    {
        auto json = repo.Select()
        .Where("age > 30")
            .OrderByAsc("id")
            .ToJson();

        qDebug().noquote() << json;
    }

    bool Insert(Person person)
    {

       return repo.Insert(person);
    }

    void Update()
    {

    }

    bool Delete(QString clause)
    {
        return repo.Delete(clause);
    }

private:
    Q1Connection* connection;
    Person repo;
};


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    const QString host = "localhost";
    const QString dbName = "testDb";
    const QString dbUser = "postgres";
    const QString dbPass = "123";
    const int port = 5432;

    Q1Connection conn(Q1Driver::POSTGRE_SQL, host, dbName, dbUser, dbPass, port);


    // Use Person as repository
    Person repo(&conn);




    // auto list = repo.Select()
    //                 .Where("age > 30")
    //                 .OrderByAsc("id")
    //                 .ToList();

    // auto json = repo.Select()
    //                       .Where("age > 30")
    //                       .OrderByAsc("id")
    //                       .ToJson();

    // qDebug().noquote() << json;

    // for (const PersonDto& p : list)
    //     qDebug() << p.id << p.first_name << p.last_name << p.age << p.address_id;

    Crud crud(&conn);

    Person user(&conn);

    user.first_name = "michael";
    user.last_name = "jackson";
    user.address_id = 0;
    user.age = 10;


    crud.Insert(user);

    // crud.Delete("age = 60");

    crud.GetByJson();





    conn.Disconnect();
    return a.exec();
}

