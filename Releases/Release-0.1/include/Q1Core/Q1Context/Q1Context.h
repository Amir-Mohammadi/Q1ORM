#ifndef Q1CONTEXT_H
#define Q1CONTEXT_H

#include <QCoreApplication>
#include <QString>
#include <QObject>
#include <QSqlDatabase>
#include <QtSql>

#include "Q1Core/Q1Entity/Q1Table.h"
#include "Q1Core/Q1Entity/Q1Entity.h"
#include "Q1Core/Q1Context/Q1Connection.h"
#include "Q1Core/Q1Migration/Q1Migration.h"

#include "Q1ORM_global.h"

class Q1ORM_EXPORT Q1Context : public QObject
{
    Q_OBJECT
public:
    Q1Context() { }
    ~Q1Context();

public: //Virtual
    virtual void OnConfiguration() = 0;
    virtual QList<Q1Table> OnTablesCreating() = 0;

public: //Methods
    void Initialize();

public: //Setter
    void ColumnsMigration(bool status)
    {
        check_columns = status;
    }

private: //Methods
    void InitialDatabase();
    void InitialTables();
    void InitialColumns(Q1Table &q1table);
    void CompareColumn(QString table_name, Q1Column &column, Q1Column &q1column);

protected:
    Q1Connection* connection = nullptr;
    Q1Migration* query = nullptr;

private:
    QString database_name;
    QList<Q1Table> q1tables;

    QStringList tables;

private:
    bool check_columns = false;
};

#endif // Q1CONTEXT_H
