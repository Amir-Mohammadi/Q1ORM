#include <QCoreApplication>
#include <QObject>
#include <QDir>

#include <Q1DatabaseInstall/Q1DatabaseInstall.h>

class ExampleProcess : public QObject
{
    Q_OBJECT

public:
    ExampleProcess(Q1DatabaseInstall *database_install) : _database_install(database_install)
    {
        connect(database_install, &Q1DatabaseInstall::InstallingStatus, this, &ExampleProcess::HandleInstallingStatus);
    }

public slots:
    void HandleInstallingStatus(Q1DatabaseInstallationStatus status ,QString message)
    {
        qDebug() << "status code :" << status << "|" << message;
    }

private:
    Q1DatabaseInstall *_database_install;
};


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    Q1DatabaseInstall database_install;
    ExampleProcess dataProcessor(&database_install);

    database_install.InstallPostgreSql("C:/", "C:/", "admin", "Admin-123");



    return a.exec();
}

#include "main.moc"
