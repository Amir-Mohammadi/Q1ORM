#ifndef Q1DATABASEINSTALL_H
#define Q1DATABASEINSTALL_H

#include <QDebug>
#include <QString>
#include <QProcess>
#include <QObject>
#include <QByteArray>
#include <QDir>

#include "Q1ORM_global.h"

enum Q1DatabaseInstallationStatus
{
    ArgumentsWrong = 0,
    DownloadPathWrong,
    ExtractPathWrong,
    CheckingDBFolder,
    DBAlreadyInstalled,
    CheckingZipFile,
    ZipFileExists,
    DownloadZipFile,
    DownloadFail,
    ZipFileNotExists,
    ExtractingZipFile,
    ClusterDbHasExist,
    ExtractionComplete,
    Complete,
    Error
};

class Q1ORM_EXPORT Q1DatabaseInstall : public QObject
{
    Q_OBJECT

public:
    Q1DatabaseInstall(QObject *parent = nullptr);

    void InstallPostgreSql(QString download_to, QString extract_to, QString username, QString password);

private:
    void CreateBatchFile();
    void InstallEventHandler(QString output);

public slots:
    void ProcessBatchOutput();

signals:
    void InstallingStatus(Q1DatabaseInstallationStatus status, QString message);

private:
    QProcess process;

    QRegExp process_output_regx;
    QStringList process_output_numbers = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13"};

    QString batch_file_directory = QDir::currentPath();
    QString postgre_batch_name = "postgresql_install.bat";

    QString postgre_batch_content = "@echo off\ncls\n@REM Checking the number of input arguments.\nset arg_count=0\nfor %%x in (%*) do Set /A arg_count+=1\nif NOT \"%arg_count%\" == \"4\" (\n    echo \"0|The number of input arguments should be 4 including (file path or file download location, extract path, username, password)\"\n    pause\n    exit /b 0\n)\nset download_to=%1\nset extract_to=%2\nset username=%3\nset password=%4\nif NOT EXIST \"%download_to%\" (\n    echo \"1|The file path or file download path is not correct.\"\n    pause\n    exit /b 0\n)\nif NOT EXIST \"%extract_to%\" (\n    echo \"2|The path of the desired location to extract the file is not correct.\"\n    pause\n    exit /b 0\n)\n@REM Checks if the last character is equal to \"/\" or \"\\\" and deletes that character. //// for download_to\nset download_to_last_character=%download_to:~-1%\nif /I \"%download_to_last_character%\" == \"/\" (\n    set download_to=%download_to:~0,-1%\n) else if /I \"%download_to_last_character%\" == \"\\\" (\n    set download_to=%download_to:~0,-1%\n)\n@REM Checks if the last character is equal to \"/\" or \"\\\" and deletes that character. //// for extract_to\nset extract_to_last_character=%extract_to:~-1%\nif /I \"%extract_to_last_character%\" == \"/\" (\n    set extract_to=%extract_to:~0,-1%\n) else if /I \"%extract_to_last_character%\" == \"\\\" (\n    set extract_to=%extract_to:~0,-1%\n)\nset \"zip_file=%download_to%\\postgresql.zip\"\nset \"cluster_db=%extract_to%\\pgsql\\bin\\clusterdb.exe\"\nset \"psql_folder=%extract_to%\\pgsql\\bin\"\nset \"data_folder=%extract_to%\\pgsql\\data\"\necho \"3|Checking postgresql folder is exists ...\"\nif EXIST \"%cluster_db%\" (\n  echo \"4|PostgreSQL is already installed at \"%extract_to%/\"\"\n  pause\n  exit /b 0\n)\necho \"5|Checking that the zip file exists (postgresql.zip) ...\"\ncd /D \"%download_to%/\"\nif EXIST \"%zip_file%\" (\n    echo \"6|postgresql.zip file is exist.\"\n) else (\n    echo \"7|Start downloading postgresql.zip ...\"\n    powershell -Command \"Invoke-WebRequest https://sbp.enterprisedb.com/getfile.jsp?fileid=1258697 -Outfile postgresql.zip\"\n    if ERRORLEVEL 1 (\n        echo \"8|Download Fail, Please Check Your Internet Connection!\"\n        DEL \"%zip_file%\"\n        pause\n        exit /b 1\n    )\n)\nif NOT EXIST \"%zip_file%\" (\n    echo \"9|The Zip File \"%zip_file%\" does not exist.\"\n    pause\n    exit /b 0\n)\nif not exist \"%extract_to%/\" (\n    mkdir \"%extract_to%/\"\n)\necho \"10|Extracting ...\"\nif EXIST \"%cluster_db%\" (\n    echo \"11|Clusterdb has exist.\"\n) else (\n    powershell -nologo -noprofile -command \"& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%zip_file%', '%extract_to%/'); }\"\n)\ndel \"%zip_file%\"\necho \"12|Extraction Complete.\"\ncd /D \"%psql_folder%\"\necho %password%>> \"pass.conf\"\ncall initdb.exe -U %username% -A password --pwfile=pass.conf -E utf8 -D ..\\data\ncall pg_ctl -D ^\"^.^.^\\data^\" -l logfile start\ncall pg_ctl.exe register -N PostgreSQL -D %data_folder%\ndel \"pass.conf\"\necho \"13|All Process Done Successfuly!\"\npause\nexit /b 0";
};

#endif // Q1DATABASEINSTALL_H
