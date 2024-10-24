#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QThread>


void ConfigReleaseFolder()
{
    QString path = "./../";

    try
    {
        path = path.replace('\\', '/');

        QProcess process;
        QString command = "cmake";
        QStringList arguments = {"-S", "./", "-B", "./../build-Q1ORM-Desktop_Qt_5_15_2_MSVC2019_64bit-Release"};

        process.setWorkingDirectory(path);
        process.start(command, arguments);

        if (process.waitForStarted())
        {
            if (process.waitForFinished())
            {
                QByteArray output = process.readAllStandardOutput();
                qDebug() << output;

                qDebug() << "";
                qDebug() << "config successfully!";
            }
            else
            {
                qDebug() << "Command execution failed!";
            }
        }
        else
        {
            qDebug() << "Failed to start the command!";
        }
    }
    catch (...)
    {
        qDebug() << "There was a problem with the release!";
    }
}

void CleanProject()
{
    QString path = "./../";

    try
    {
        path = path.replace('\\', '/');

        QProcess process;
        QString command = "cmake";
        QStringList arguments = {"--build", "./../build-Q1ORM-Desktop_Qt_5_15_2_MSVC2019_64bit-Release", "--target", "clean", "--config", "release"};

        process.setWorkingDirectory(path);
        process.start(command, arguments);

        if (process.waitForStarted())
        {
            if (process.waitForFinished())
            {
                QByteArray output = process.readAllStandardOutput();
                qDebug() << output;

                qDebug() << "";
                qDebug() << "clean successfully!";
            }
            else
            {
                qDebug() << "Command execution failed!";
            }
        }
        else
        {
            qDebug() << "Failed to start the command!";
        }
    }
    catch (...)
    {
        qDebug() << "There was a problem with the release!";
    }
}

void BuildProject()
{
    QString path = "./../";

    try
    {
        path = path.replace('\\', '/');

        QProcess process;
        QString command = "cmake";
        QStringList arguments = {"--build", "./../build-Q1ORM-Desktop_Qt_5_15_2_MSVC2019_64bit-Release", "--target", "Src", "--config", "release"};

        process.setWorkingDirectory(path);
        process.start(command, arguments);

        if (process.waitForStarted())
        {
            if (process.waitForFinished())
            {
                QByteArray output = process.readAllStandardOutput();
                qDebug() << output;

                qDebug() << "";
                qDebug() << "build successfully!";
            }
            else
            {
                qDebug() << "Command execution failed!";
            }
        }
        else
        {
            qDebug() << "Failed to start the command!";
        }
    }
    catch (...)
    {
        qDebug() << "There was a problem with the release!";
    }
}

void InstallRelease()
{
    QString path = "./../../build-Q1ORM-Desktop_Qt_5_15_2_MSVC2019_64bit-Release/Src";

    try
    {
        path = path.replace('\\', '/');

        QProcess process;
        QString command = "cmake";
        QStringList arguments = {"--install", "./"};

        process.setWorkingDirectory(path);
        process.start(command, arguments);

        if (process.waitForStarted())
        {
            if (process.waitForFinished())
            {
                QByteArray output = process.readAllStandardOutput();
                qDebug() << output;

                qDebug() << "";
                qDebug() << "installed successfully!";
            }
            else
            {
                qDebug() << "Command execution failed!";
            }
        }
        else
        {
            qDebug() << "Failed to start the command!";
        }
    }
    catch (...)
    {
        qDebug() << "There was a problem with the release!";
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ConfigReleaseFolder();
    CleanProject();
    BuildProject();
    InstallRelease();

    QThread::msleep(1500);

    a.exit();
    return 0;
}
