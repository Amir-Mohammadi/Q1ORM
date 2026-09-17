#include <QCoreApplication>
#include <QLoggingCategory>
#include <QTextStream>
#include <QtTest>
#include "OrmTests.h"

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    if (!qEnvironmentVariableIsSet("Q1ORM_VERBOSE"))
        QLoggingCategory::setFilterRules("*.debug=false\n*.info=false");
    QTextStream(stdout) << "Q1ORM TestExample | Backend: "
                        << qEnvironmentVariable("Q1ORM_TEST_DRIVER", "SQLITE")
                        << " | Qt drivers: " << QSqlDatabase::drivers().join(", ")
                        << Qt::endl;
    OrmTests tests;
    return QTest::qExec(&tests, argc, argv);
}
