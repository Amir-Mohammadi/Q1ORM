#include "Q1ModelBuilder.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <utility>

namespace {
Q1Relation NormalizeRelation(Q1Relation relation)
{
    if (relation.type == ONE_TO_MANY) {
        std::swap(relation.base_table, relation.top_table);
        relation.type = MANY_TO_ONE;
    }
    return relation;
}
}

QString Q1ModelBuilder::FkKey(const Q1Relation &relation)
{
    const auto normalized = NormalizeRelation(relation);
    return QString::fromUtf8(QJsonDocument(QJsonArray{
        normalized.base_table.toLower(), normalized.top_table.toLower(),
        normalized.foreign_key.toLower(), normalized.reference_key.toLower(),
        normalized.type == MANY_TO_MANY ? "junction" : "foreign-key"
    }).toJson(QJsonDocument::Compact));
}

Q1ModelBuilder &Q1ModelBuilder::AddRelation(const Q1Relation &relation)
{
    const QString key = FkKey(relation);
    if (!m_relationKeys.contains(key)) {
        m_relationKeys.insert(key);
        m_relations.append(NormalizeRelation(relation));
    }
    return *this;
}

void Q1ModelBuilder::AddTable(Q1Table *table)
{
    if (table && !m_tables.contains(table))
        m_tables.append(table);
}

bool Q1ModelBuilder::Build()
{
    const auto deferred = std::exchange(m_deferred, {});
    for (const auto &configureRelations : deferred)
        configureRelations();
    Validate();
    return !HasErrors();
}

void Q1ModelBuilder::Validate()
{
    QHash<QString, Q1Table *> tables;
    for (Q1Table *table : m_tables) {
        const QString name = table->GetName();
        if (!table->IsValid())
            m_errors << QStringLiteral("Table '%1' must have a name and columns.").arg(name);
        if (tables.contains(name.toLower()))
            m_errors << QStringLiteral("Duplicate table name: %1.").arg(name);
        tables.insert(name.toLower(), table);
        QSet<QString> columns;
        for (const auto &column : table->GetColumns()) {
            if (column.name.trimmed().isEmpty() || columns.contains(column.name.toLower()))
                m_errors << QStringLiteral("Table '%1' has an empty or duplicate column: %2.")
                                .arg(name, column.name);
            columns.insert(column.name.toLower());
        }
    }

    for (const auto &relation : m_relations) {
        if (!relation.IsValid()) {
            m_errors << QStringLiteral("Relation must specify both tables and both keys.");
            continue;
        }
        auto *child = tables.value(relation.base_table.toLower());
        auto *parent = tables.value(relation.top_table.toLower());
        if (!child || !parent) {
            m_errors << QStringLiteral("Relation %1 references an unmapped table.")
                            .arg(relation.GetConstraintName());
            continue;
        }
        if (!child->HasColumn(relation.foreign_key) || !parent->HasColumn(relation.reference_key)) {
            m_errors << QStringLiteral("Relation %1 references a missing column.")
                            .arg(relation.GetConstraintName());
            continue;
        }
        if (relation.type != MANY_TO_MANY)
            child->AddIndex({relation.foreign_key});
    }
    m_errors.removeDuplicates();
}
