#include <QCoreApplication>
#include <QDebug>
#include "Person.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Connect to the database
    Q1Connection conn(Q1Driver::POSTGRE_SQL, "localhost", "testDb", "postgres", "123", 5432);

    // Create the repository for the Person entity
    Person user(&conn);

    // Example: get average age
    auto avgAge = user.Select().Avg("age");
    qDebug() << "Average age:" << avgAge;

    // Example: get maximum age
    auto maxAge = user.Select().Max("age");
    qDebug() << "Maximum age:" << maxAge;

    // Example: get count of users
    auto totalUsers = user.Select().Count();
    qDebug() << "Total users:" << totalUsers;

    // Example: get sum of ages
    auto sumAge = user.Select().Sum("age");
    qDebug() << "Sum of ages:" << sumAge;


    qDebug() << "distinct List : " ;
    auto distinct = user.Select().Distinct().ShowList();

    return a.exec();
}
