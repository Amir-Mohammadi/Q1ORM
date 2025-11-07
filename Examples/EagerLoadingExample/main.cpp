#include <QCoreApplication>
#include <QTimer>

#include <Q1Core/Q1Context/Q1Context.h>
#include <Q1ORM.h>
#include "person.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // configure connection (adjust credentials if needed)
    Q1Connection conn(Q1Driver::POSTGRE_SQL, "localhost", "testDb", "postgres", "123", 5432);
    Person user(&conn);

    // run the DB work after Qt event loop starts, then quit
    QTimer::singleShot(0, [&]() {
        // run the select and print JSON
        auto json = user.Select()
                        .Include(QStringList() << "addresses")
                        .ShowJson();

        // if ShowJson returns something useful you can also capture it here.
        QCoreApplication::quit();
    });

    return a.exec();
}
