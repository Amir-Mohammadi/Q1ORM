#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    // Verify the packaged plugins, without falling back to the developer's Qt.
    QCoreApplication::setLibraryPaths({QCoreApplication::applicationDirPath()});
    QTextStream out(stdout);
    QTextStream err(stderr);
    const QStringList drivers = app.arguments().mid(1);
    if (drivers.isEmpty() || drivers.contains(QStringLiteral("--help")))
    {
        out << "Usage: Q1SqlDriverCheck QMYSQL [QSQLITE QPSQL QODBC]\n"
               "Loads packaged SQL drivers without connecting to a database.\n";
        return drivers.isEmpty() ? 1 : 0;
    }

    bool success = true;
    for (const QString &driver : drivers)
    {
        const QString name = QStringLiteral("q1orm-runtime-check-") + driver;
        {
            const QSqlDatabase db = QSqlDatabase::addDatabase(driver, name);
            if (db.isValid())
                out << driver << ": OK\n";
            else
            {
                err << driver << ": FAILED: " << db.lastError().text() << '\n'
                    << "Check the packaged sqldrivers plugin and native client libraries.\n"
                       "Set QT_DEBUG_PLUGINS=1 for loader diagnostics.\n";
                success = false;
            }
        }
        QSqlDatabase::removeDatabase(name);
    }
    return success ? 0 : 1;
}
