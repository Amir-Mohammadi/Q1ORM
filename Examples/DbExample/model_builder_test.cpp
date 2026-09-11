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
};

// Neither ApplyMap overload requires an EntityType alias or map base class.
struct ChildMap {
    static void ConfigureEntity(Q1Entity<Child> &entity) {
        entity.ToTableName("children");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.parent_id, "parent_id");
        entity.HasOne<Parent>().WithMany()
            .HasForeignKey<&Child::parent_id>().HasPrincipalKey<&Parent::id>();
    }
};

struct PlainMap {
    static void ConfigureEntity(Q1Entity<Parent> &entity) {
        entity.ToTableName("plain");
        entity.Property(entity.id, "id", false, true);
    }
};

// Typed maps deliberately declare the child first. Resolution must use final
// table/column overrides, without configuring a temporary principal entity.
struct TypedParentMap {
    static void ConfigureEntity(Q1Entity<Parent>& entity) {
        entity.ToTableName("typed_parents");
        entity.HasKey<&Parent::id>().ValueGeneratedOnAdd().HasColumnName("parent_key");
        entity.Property<&Parent::name>().IsRequired().HasColumnName("display_name");
    }
};

struct TypedChildMap {
    static void ConfigureEntity(Q1Entity<Child>& entity) {
        entity.HasKey<&Child::id>().ValueGeneratedOnAdd();
        entity.HasOne<Parent>().WithMany()
            .HasForeignKey<&Child::parent_id>().HasPrincipalKey<&Parent::id>();
        entity.Property<&Child::parent_id>().HasColumnName("parent_ref");
    }
};

struct UnmappedKeyMap {
    static void ConfigureEntity(Q1Entity<Child>& entity) {
        entity.HasKey<&Child::id>();
        entity.HasOne<Parent>().WithMany()
            .HasForeignKey<&Child::parent_id>().HasPrincipalKey<&Parent::id>();
    }
};

class TypedContext : public Q1Context {
public:
    explicit TypedContext(Q1Connection* connection) { SetConnection(connection); }
    Q1Entity<Parent> parents;
    Q1Entity<Child> children;
protected:
    void OnModelCreating(Q1ModelBuilder& builder) override {
        builder.ApplyMap<TypedChildMap>(children).ApplyMap<TypedParentMap>(parents);
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
    builder.AddRelation(Q1Relation("parents", "children", ONE_TO_MANY, "parent_id", "id"));
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

    CHECK(Q1Detail::MemberName<&Parent::id>() == "id");
    CHECK(Q1Detail::MemberName<&Child::parent_id>() == "parent_id");
    TypedContext typed(&connection);
    Q1ModelBuilder typedBuilder(&typed);
    typedBuilder.ApplyMap<TypedChildMap>(typed.children).ApplyMap<TypedParentMap>(typed.parents);
    CHECK(typedBuilder.Build());
    CHECK(typedBuilder.Relations().size() == 1);
    const auto typedRelation = typedBuilder.Relations().first();
    CHECK(typedRelation.base_table == "Child"); // Default: exact C++ type name.
    CHECK(typedRelation.top_table == "typed_parents");
    CHECK(typedRelation.foreign_key == "parent_ref");
    CHECK(typedRelation.reference_key == "parent_key");
    CHECK(typed.children.GetTablePtr()->GetIndexes().first().columns == QStringList{"parent_ref"});
    CHECK(typed.parents.GetTablePtr()->FindColumn("parent_key")->is_identity);
    CHECK(!typed.parents.GetTablePtr()->FindColumn("display_name")->nullable);
    CHECK(typedBuilder.Build());
    CHECK(typedBuilder.Relations().size() == 1);
    CHECK(typed.Initialize());
    Parent typedParent;
    typedParent.name = "typed mapping";
    CHECK(typed.parents.Insert(typedParent));
    CHECK(typedParent.id > 0);
    Child typedChild;
    typedChild.parent_id = typedParent.id;
    CHECK(typed.children.Insert(typedChild));
    CHECK(typedChild.id > 0);
    CHECK(typed.parents.SelectAll().first().name == typedParent.name);
    CHECK(typed.children.SelectAll().first().parent_id == typedParent.id);

    Q1Entity<Child> orphan;
    Q1ModelBuilder missingPrincipal;
    missingPrincipal.ApplyMap<TypedChildMap>(orphan);
    CHECK(!missingPrincipal.Build());
    CHECK(!missingPrincipal.Errors().isEmpty());

    Q1Entity<Child> unmappedKey;
    Q1Entity<Parent> mappedParent;
    Q1ModelBuilder missingKey;
    missingKey.ApplyMap<UnmappedKeyMap>(unmappedKey).ApplyMap<TypedParentMap>(mappedParent);
    CHECK(!missingKey.Build());
    CHECK(!missingKey.Errors().isEmpty());

    Q1Entity<Parent> conventional;
    conventional.HasKey<&Parent::id>();
    conventional.Property<&Parent::name>().IsRequired(false);
    CHECK(conventional.GetTablePtr()->GetName() == "Parent");
    CHECK(conventional.GetTablePtr()->FindColumn("name")->nullable);
    CHECK(conventional.ColumnName<&Parent::name>() == "name");
    bool duplicateRejected = false;
    try { conventional.Property<&Parent::name>().HasColumnName("id"); }
    catch (const std::invalid_argument&) { duplicateRejected = true; }
    CHECK(duplicateRejected);

    InvalidContext invalid(&connection);
    CHECK(!invalid.Initialize());
    CHECK(invalid.GetLastError().contains("not registered"));
    return 0;
}
