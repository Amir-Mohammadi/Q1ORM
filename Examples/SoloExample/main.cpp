#include "applicationdbcontext.h"
#include <Q1Core/Q1Context/Q1Connection.h>
#include <QCoreApplication>

#include <Q1ORM.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Q1Connection* conn = new Q1Connection(
        Q1Driver::POSTGRE_SQL,
        "localhost",
        "solo",
        "postgres",
        "123"
        );

    if (!conn->RootConnect() || !conn->Connect())
    {
        qDebug() << "Database connection failed.";
        return 1;
    }

    // Initialize ORM Context
    ApplicationDbContext ctx(conn);
    ctx.Initialize();



    conn->Disconnect();
    conn->RootDisconnect();

    return a.exec();
}
