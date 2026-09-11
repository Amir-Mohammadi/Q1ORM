#include <Q1ORM.h>
#include <QCoreApplication>
#include <QTemporaryDir>

struct Parent { int id = 0; QString name; };
struct Child { int id = 0; int parent_id = 0; };

struct ParentMap {
    static void ConfigureEntity(Q1Entity<Parent> &entity) {
        entity.ToTableName("parents");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.name, "name");
    }
    static QList<Q1Relation> CreateRelations(Q1Entity<Parent> &entity) {
        return {entity.Relations("parents", "children", ONE_TO_MANY, "parent_id", "id")};
    }
};

// Neither ApplyMap overload requires an EntityType alias or map base class.
struct ChildMap {
    static void ConfigureEntity(Q1Entity<Child> &entity) {
        entity.ToTableName("children");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.parent_id, "parent_id");
    }
    static QList<Q1Relation> CreateRelations(Q1Entity<Child> &entity) {
        return {entity.Relations("children", "parents", MANY_TO_ONE, "parent_id", "id")};
    }
};

struct PlainMap {
    static void ConfigureEntity(Q1Entity<Parent> &entity) {
        entity.ToTableName("plain");
        entity.Property(entity.id, "id", false, true);
    }
};

class TestContext : public Q1Context {
public:
    explicit TestContext(Q1Connection *connection) {
        RegisterEntity(&parents); // Also exercise registration before SetConnection.
        SetConnection(connection);
    }
    Q1Entity<Parent> parents;
    Q1Entity<Child> children;
protected:
    void OnModelCreating(Q1ModelBuilder &builder) override {
        builder.ApplyMap<ChildMap>(children).ApplyMap<ParentMap>();
    }
};

class InvalidContext : public Q1Context {
public:
    explicit InvalidContext(Q1Connection *connection) { SetConnection(connection); }
protected:
    void OnModelCreating(Q1ModelBuilder &builder) override { builder.ApplyMap<ParentMap>(); }
};

#define CHECK(condition) do { if (!(condition)) { \
    qCritical() << "Check failed at line" << __LINE__ << #condition; return 1; \
} } while (false)

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    CHECK(directory.isValid());
    Q1Connection connection(Q1Driver::SQLITE, "", directory.filePath("model.sqlite"), "", "", 0);
    TestContext context(&connection);
    Q1ModelBuilder builder(&context);
    builder.ApplyMap<ChildMap>(context.children).ApplyMap<ParentMap>();
    builder.ApplyMap<ChildMap>();
    CHECK(builder.Build());
    CHECK(builder.Tables().size() == 2);
    CHECK(builder.Relations().size() == 1);
    CHECK(builder.Relations().first().base_table == "children");
    CHECK(context.children.GetTablePtr()->GetIndexes().size() == 1);
    CHECK(context.parents.GetTablePtr()->GetIndexes().isEmpty());
    CHECK(builder.Build());
    CHECK(builder.Relations().size() == 1);
    CHECK(context.ResolveEntity<Child>() == &context.children);

    CHECK(context.Initialize());
    Parent parent;
    parent.name = "mapped parent";
    CHECK(context.parents.Insert(parent));
    CHECK(parent.id > 0);
    Child child;
    child.parent_id = parent.id;
    CHECK(context.children.Insert(child));
    const auto parents = context.parents.SelectAll();
    CHECK(parents.size() == 1);
    CHECK(parents.first().name == parent.name);
    CHECK(context.children.SelectAll().size() == 1);
    CHECK(context.Initialize());
    CHECK(context.parents.SelectAll().size() == 1);

    Q1Entity<Parent> plain;
    Q1ModelBuilder plainBuilder;
    plainBuilder.ApplyMap<PlainMap>(plain);
    CHECK(plainBuilder.Build());
    CHECK(plainBuilder.Relations().isEmpty());

    Q1ModelBuilder missing;
    missing.ApplyMap<ParentMap>();
    CHECK(!missing.Build());
    CHECK(!missing.Errors().isEmpty());
    const auto errorCount = missing.Errors().size();
    CHECK(!missing.Build());
    CHECK(missing.Errors().size() == errorCount);

    Q1Entity<Parent> duplicate;
    plainBuilder.ApplyMap<PlainMap>(duplicate);
    CHECK(!plainBuilder.Build());

    Q1ModelBuilder badRelation;
    badRelation.ApplyMap<PlainMap>(plain);
    badRelation.AddRelation(Q1Relation("plain", "absent", MANY_TO_ONE, "id", "id"));
    CHECK(!badRelation.Build());

    Q1ModelBuilder badColumn;
    badColumn.ApplyMap<PlainMap>(plain);
    badColumn.AddRelation(Q1Relation("plain", "plain", MANY_TO_ONE, "missing", "id"));
    CHECK(!badColumn.Build());

    InvalidContext invalid(&connection);
    CHECK(!invalid.Initialize());
    CHECK(invalid.GetLastError().contains("not registered"));
    return 0;
}
