#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QDir>


QString CURRENT_DIR = QDir::currentPath();
QString EXAMPLES_DIR = QDir::currentPath() + "/../Examples";
QString TEMPLATE_DIR = QDir::currentPath() + "/Template";


void FixProjectFile(QString project_name, QString path)
{
    QString file_path = path + "/CMakeLists.txt";

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open the project file for reading.";
        return;
    }

    QTextStream in(&file);
    QString file_content = in.readAll();
    file.close();

    file_content.replace("{ExampleProjectName}", project_name);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open the file for writing.";
        return;
    }

    QTextStream out(&file);
    out << file_content;
    file.close();
}

void FixSubProjectsFile(QString project_name, bool is_add)
{
    QString file_path = EXAMPLES_DIR + "/CMakeLists.txt";

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open the project file for reading.";
        return;
    }

    QTextStream in(&file);
    QString file_content = in.readAll();
    file.close();

    if(is_add)
    {
        file_content.replace("subdirs(", "subdirs(" + project_name + " ");
    }
    else
    {
        file_content.replace(project_name + " ", "");
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open the file for writing.";
        return;
    }

    QTextStream out(&file);
    out << file_content;
    file.close();
}


QString CreateFolder(QString project_name)
{
    QString folder_path = EXAMPLES_DIR + "/" + project_name;

    if (!QDir(folder_path).exists())
    {
        if (!QDir().mkpath(folder_path))
        {
            qDebug() << "Failed to create folder.";
        }
    }
    else
    {
        qDebug() << "Folder already exists.";
    }

    return folder_path;
}

void CreateFiles(QString project_name, QString path)
{
    QString project_file_name = "CMakeLists.txt";
    QString main_file_name = "main.cpp";
    QString ignore_file_name = ".gitignore";

    QFile::copy(TEMPLATE_DIR + "/" + ignore_file_name,
                path + "/" + ignore_file_name);

    QFile::copy(TEMPLATE_DIR + "/" + main_file_name,
                path + "/" + main_file_name);

    QFile::copy(TEMPLATE_DIR + "/" + project_file_name,
                path + "/" + project_file_name);
}

void RemoveProject(QString project_name)
{
    QString folder_path = EXAMPLES_DIR + "/" + project_name;
    QDir directory(folder_path);

    if (directory.exists())
    {
        if (!directory.removeRecursively())
        {
            qDebug() << "There was a problem deleting the project!";
        }
        else
        {
            FixSubProjectsFile(project_name, false);
            qDebug() << "Project removed successfully!";
        }
    }
}


void GetCommand()
{
    QRegExp pattern("^([A-Za-z]:)?[^/:*?\"<>|]+\\b");
    QTextStream input(stdin);

    QString project_name = "";
    QString command = "";
    QStringList commands;

    bool is_valid = false;

    while(project_name.trimmed().length() == 0 && !is_valid)
    {
        qDebug() << "Enter your command:";
        input.flush();

        command = input.readLine();
        commands = command.split(" ");

        if(commands.length() != 2)
        {
            qDebug() << "This command is wrong!";
        }

        if(commands[0].toLower() != "add" && commands[0].toLower() != "remove")
        {
            qDebug() << "This command is wrong!";
        }

        project_name = commands[1];
        is_valid = pattern.exactMatch(project_name);
    }

    project_name = project_name + "Example";

    if(commands[0].toLower() == "add")
    {
        QString folder_path = CreateFolder(project_name);

        CreateFiles(project_name, folder_path);
        FixProjectFile(project_name, folder_path);
        FixSubProjectsFile(project_name, true);

        qDebug() << "Project created successfully!";
    }
    else
    {
        RemoveProject(project_name);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "Notes:";
    qDebug() << "Write your command in this format: {Add/Remove} {Project Name}";
    qDebug() << "Example: Add ConnectToDatabase";
    qDebug() << "Don't use space in project name.";
    qDebug() << "";

    try
    {
        GetCommand();
    }
    catch (...)
    {
        qDebug() << "There was a problem with the generate example project!";
    }

    QThread::msleep(1500);

    a.exit();
    return 0;
}
