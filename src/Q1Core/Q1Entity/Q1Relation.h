#ifndef Q1RELATION_H
#define Q1RELATION_H


#include <QString>
#include <qdebug.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <qsqlquery.h>

#include "../../Q1ORM_global.h"

enum Q1RelationType
{
    ONE_TO_ONE,      // 1:1 relationship (e.g., User -> Profile)
    ONE_TO_MANY,     // 1:* relationship (e.g., User -> Posts)
    MANY_TO_ONE,     // *:1 relationship (e.g., Posts -> User)
    MANY_TO_MANY     // *:* relationship (e.g., Students <-> Courses)
};


enum Q1CascadeAction
{
    NO_ACTION,       // Do nothing on delete/update
    CASCADE,         // Delete/update related rows
    SET_NULL,        // Set foreign key to NULL
    SET_DEFAULT,     // Set foreign key to default value
    RESTRICT         // Prevent delete/update if related rows exist
};

class Q1ORM_EXPORT Q1Relation
{
public:
    Q1Relation()
        : type(ONE_TO_MANY),
        on_delete(CASCADE),
        on_update(NO_ACTION),
        lazy_load(true)
    {}

    Q1Relation(const QString& base_table,
               const QString& top_table,
               Q1RelationType rel_type,
               const QString& foreign_key_col,
               const QString& reference_key_col = "id")
        :
        base_table(base_table),
        top_table(top_table),
        foreign_key(foreign_key_col),
        reference_key(reference_key_col),
        type(rel_type),
        on_delete(CASCADE),
        on_update(NO_ACTION),
        lazy_load(true)
    {}




    //Get ConstraintExist
    bool ConstraintExist(QSqlDatabase &db) const
    {
        if (!db.isOpen()) {
            qWarning() << "ConstraintExists: database not open";
            return false;
        }

        QSqlQuery q(db);
        q.prepare(R"(SELECT COUNT(*) FROM information_schema.table_constraints
                    WHERE constraint_name = :cname)");
        q.bindValue(":cname", GetConstraintName());

        if (!q.exec()) {
            qWarning() << "ConstraintExists query failed:" << q.lastError().text();
            return false;
        }

        if (q.next()) {
            return q.value(0).toInt() > 0;
        }

        return false;
    }


    // Set cascade behavior
    void SetOnDelete(Q1CascadeAction action)
    {
        on_delete = action;
    }

    void SetOnUpdate(Q1CascadeAction action)
    {
        on_update = action;
    }

    // Get SQL cascade string
    QString GetOnDeleteString() const
    {
        return GetCascadeString(on_delete);
    }

    QString GetOnUpdateString() const
    {
        return GetCascadeString(on_update);
    }

    // Set loading strategy
    void SetLazyLoad(bool lazy)
    {
        lazy_load = lazy;
    }

    // Set back reference name (for navigation property)
    void SetBackReference(const QString& name)
    {
        back_reference = name;
    }

    // Get constraint name
    QString GetConstraintName() const
    {
        return "FK_" + base_table + "_" + top_table + "_" + foreign_key;
    }

    // Validate relation
    bool IsValid() const
    {
        return !base_table.isEmpty() &&
               !top_table.isEmpty() &&
               !foreign_key.isEmpty() &&
               !reference_key.isEmpty();
    }

private:

    QString GetCascadeString(Q1CascadeAction action) const
    {
        switch (action)
        {
        case NO_ACTION:   return "NO ACTION";
        case CASCADE:     return "CASCADE";
        case SET_NULL:    return "SET NULL";
        case SET_DEFAULT: return "SET DEFAULT";
        case RESTRICT:    return "RESTRICT";
        default:          return "NO ACTION";
        }
    }


public:

    QString base_table;         // Table containing the foreign key
    QString top_table;          // Referenced/parent table
    QString foreign_key;        // Foreign key column name in base_table
    QString reference_key;      // Referenced column in top_table (usually "id")
    Q1RelationType type;
    Q1CascadeAction on_delete;
    Q1CascadeAction on_update;

    // Navigation properties
    bool lazy_load;             // Load related data on demand vs eagerly
    QString back_reference;     // Name for reverse navigation (e.g., "user" for posts.user)
};

#endif // Q1RELATION_H
