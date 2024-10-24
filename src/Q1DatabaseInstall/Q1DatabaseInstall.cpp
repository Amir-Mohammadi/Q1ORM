#include "Q1DatabaseInstall.h"

Q1DatabaseInstall::Q1DatabaseInstall(QObject *parent) : QObject{parent}
{
    process_output_regx.setPattern("(\\d+\\|[^.]+)");

    this->CreateBatchFile();
}

void Q1DatabaseInstall::InstallPostgreSql(QString download_to, QString extract_to, QString username, QString password)
{
    try
    {
        QString command = "%1 %2 %3 %4 %5";
        command = command.arg(postgre_batch_name, download_to, extract_to, username, password);
        
        connect(&process, &QProcess::readyReadStandardOutput, this, &Q1DatabaseInstall::ProcessBatchOutput);
        process.setWorkingDirectory(batch_file_directory);

        process.start("cmd.exe", QStringList() << "/c" << command);

        if (process.waitForStarted())
        {
            if(process.waitForFinished(-1))
            {
                return;
            }
        }
    }
    catch (...) { }

    emit InstallingStatus(Q1DatabaseInstallationStatus::Error, "There was a problem with the database installation!");
}

void Q1DatabaseInstall::ProcessBatchOutput()
{
    QByteArray process_output = process.readAllStandardOutput();
    QString output_text = QTextStream(process_output).readAll();

    output_text = output_text.replace("\f", "");
    output_text = output_text.replace("\\", "");
    output_text = output_text.replace("\"", "");

    QStringList outputs = output_text.split("\r\n");

    for(QString output : outputs)
    {
        if(output.trimmed().length() != 0)
        {
            InstallEventHandler(output);
        }
    }
}

/* ====================================================================================================================== */
/* ====================================================== PRIVATE ======================================================= */
/* ====================================================================================================================== */

void Q1DatabaseInstall::CreateBatchFile()
{
    QFile file(batch_file_directory + "/" + postgre_batch_name);

    if (!file.exists())
    {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream stream(&file);
            stream << postgre_batch_content;

            file.close();
            return;
        }
    }

    emit InstallingStatus(Q1DatabaseInstallationStatus::Error, "Failed to create the batch file!");
}

void Q1DatabaseInstall::InstallEventHandler(QString output)
{
    int offset = process_output_regx.indexIn(output);

    if (offset == -1)
    {
        return;
    }

    QStringList output_args = output.split('|');
    int index_of = process_output_numbers.indexOf(output_args[0]);

    emit InstallingStatus((Q1DatabaseInstallationStatus)index_of, output_args[1]);

    switch(index_of)
    {
        case 0:
        case 1:
        case 2:
        case 4:
        case 8:
        case 9:
        case 13:
        {
        disconnect(&process, &QProcess::readyReadStandardOutput, this, &Q1DatabaseInstall::ProcessBatchOutput);
            process.close();

            break;
        }
    }
}
