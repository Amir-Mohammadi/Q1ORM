#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QProcessEnvironment>
#include <QLibraryInfo>
#include <QDir>
#include <QFile>

// Auto-detected from the Qt build this program was compiled with,
// so it works regardless of where Qt 6 is installed on the machine.
static QString qtBinPath;
static QString qtPrefixPath;

bool RunProcess(const QString &program,
                const QStringList &arguments,
                const QString &workingDirectory)
{
    QProcess process;

    process.setWorkingDirectory(workingDirectory);

    // Make sure the Qt runtime DLLs (and Qt-based tools) can be found.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    if (!qtBinPath.isEmpty() && !path.contains(qtBinPath))
        env.insert("PATH", qtBinPath + ";" + path);
    process.setProcessEnvironment(env);

    qDebug() << "\n================================";
    qDebug() << "Command:" << program;
    qDebug() << "Arguments:" << arguments;
    qDebug() << "Directory:" << workingDirectory;
    qDebug() << "================================\n";


    process.start(program, arguments);


    if (!process.waitForStarted())
    {
        qDebug() << "FAILED TO START:";
        qDebug() << process.errorString();
        return false;
    }


    process.waitForFinished(-1);


    QByteArray output = process.readAllStandardOutput();
    QByteArray error = process.readAllStandardError();


    if (!output.isEmpty())
        qDebug().noquote() << output;


    if (!error.isEmpty())
        qDebug().noquote() << error;


    qDebug() << "Exit Code:" << process.exitCode();


    if (process.exitStatus() != QProcess::NormalExit ||
        process.exitCode() != 0)
    {
        qDebug() << "FAILED!";
        return false;
    }


    qDebug() << "SUCCESS!";
    return true;
}



int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);


    /*
        Example structure:

        Q1ORM
        |
        |-- CMakeLists.txt
        |-- Src
        |-- Builder
             |
             |-- Q1ORMBuilder.exe

    */


    // Locate the Qt installation this program was built with.
    qtBinPath =
        QLibraryInfo::path(QLibraryInfo::BinariesPath);
    qtPrefixPath =
        QDir(QDir(qtBinPath).absoluteFilePath("..")).canonicalPath();

    qDebug() << "Qt Bin Path:" << qtBinPath;
    qDebug() << "Qt Prefix Path:" << qtPrefixPath;


    QString builderPath =
        QCoreApplication::applicationDirPath();


    QString projectPath =
        QDir(builderPath)
            .absoluteFilePath("../");


    projectPath =
        QDir(projectPath)
            .canonicalPath();



    QString buildPath =
        QDir(projectPath)
            .absoluteFilePath(
                "build-Q1ORM-Qt6-MSVC2022-Release"
                );



    qDebug() << "Project Path:";
    qDebug() << projectPath;


    qDebug() << "Build Path:";
    qDebug() << buildPath;



    /*
        1. CMake Configure
    */

    if(!RunProcess(
            "cmake",
            {
                "-S",
                projectPath,

                "-B",
                buildPath,

                "-DCMAKE_BUILD_TYPE=Release",
                "-DCMAKE_PREFIX_PATH=" + QDir::toNativeSeparators(qtPrefixPath)
            },
            projectPath))
    {
        qDebug() << "CONFIGURE FAILED!";
        return 1;
    }



    /*
        2. Clean
    */

    if(!RunProcess(
            "cmake",
            {
                "--build",
                buildPath,

                "--target",
                "clean",

                "--config",
                "Release"
            },
            projectPath))
    {
        qDebug() << "CLEAN FAILED!";
        return 1;
    }




    /*
        3. Build Library / DLL + Example executables
    */

    if(!RunProcess(
            "cmake",
            {
                "--build",
                buildPath,

                "--target",
                "Src",

                "--target",
                "DatabaseInstallExample",

                "--target",
                "SoloExample",

                "--target",
                "UnitTestExample",

                "--config",
                "Release"
            },
            projectPath))
    {
        qDebug() << "BUILD FAILED!";
        return 1;
    }




    /*
        5. Install
    */

    if(!RunProcess(
            "cmake",
            {
                "--install",
                buildPath,

                "--config",
                "Release",

                "--component",
                "Q1ORM_Library"
            },
            projectPath))
    {
        qDebug() << "INSTALL FAILED!";
        return 1;
    }



    /*
        6. Deploy example executables into the release bin directory
    */

    QString releaseBinPath =
        QDir(QDir(QDir(projectPath).absoluteFilePath("Releases"))
                 .absoluteFilePath("Release-0.1"))
            .absoluteFilePath("bin");

    QDir releaseBinDir(releaseBinPath);
    if (!releaseBinDir.exists())
    {
        qDebug() << "RELEASE BIN DIRECTORY MISSING!";
        qDebug() << releaseBinPath;
        return 1;
    }

    QStringList exampleExecutables = {};

    struct ExeDeploy
    {
        QString source;
        QString fileName;
    };

    QList<ExeDeploy> deployList =
    {
        {
            QDir(buildPath).absoluteFilePath("src/Release/Q1ORM.dll"),
            "Q1ORM.dll"
        },
        {
            QDir(buildPath)
                .absoluteFilePath("Examples/DatabaseInstallExample/Release/DatabaseInstallExample.exe"),
            "DatabaseInstallExample.exe"
        },
        {
            QDir(buildPath)
                .absoluteFilePath("Examples/SoloExample/Release/SoloExample.exe"),
            "SoloExample.exe"
        },
        {
            QDir(buildPath)
                .absoluteFilePath("bin/Release/UnitTestExample.exe"),
            "UnitTestExample.exe"
        }
    };

    for (const ExeDeploy &entry : deployList)
    {
        if (!QFile::exists(entry.source))
        {
            qDebug() << "DEPLOY SOURCE MISSING:" << entry.source;
            return 1;
        }

        QString destination =
            QDir(releaseBinDir).absoluteFilePath(entry.fileName);
        QDir().remove(destination);
        if (!QFile::copy(entry.source, destination))
        {
            qDebug() << "DEPLOY COPY FAILED:"
                     << entry.source << "->" << destination;
            return 1;
        }
        qDebug() << "Deployed:" << destination;
    }



    /*
        7. Deploy Qt runtime (Qt6Core, Qt6Sql, SQL plugins) via windeployqt
    */

    QString windeployqt = QDir(qtBinPath).absoluteFilePath("windeployqt.exe");

    QString deployTarget =
        QDir(releaseBinDir).absoluteFilePath("DatabaseInstallExample.exe");

    if(!RunProcess(
            windeployqt,
            {
                "--no-translations",
                "--no-opengl-sw",
                "--force",
                deployTarget
            },
            projectPath))
    {
        qDebug() << "QT RUNTIME DEPLOY FAILED!";
        return 1;
    }



    qDebug() << "\n==============================";
    qDebug() << "Q1ORM BUILD COMPLETED";
    qDebug() << "==============================";


    return 0;
}