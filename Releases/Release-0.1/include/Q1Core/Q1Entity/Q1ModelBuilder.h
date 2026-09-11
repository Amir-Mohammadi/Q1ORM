#ifndef Q1MODELBUILDER_H
#define Q1MODELBUILDER_H

#include <QList>
#include <QSet>
#include <QStringList>
#include <functional>
#include <type_traits>

#include "Q1Core/Q1Context/Q1Context.h"

namespace Q1Detail {
template <typename> struct MapEntity {};

template <typename TResult, typename TEntity>
struct MapEntity<TResult (*)(Q1Entity<TEntity> &)> {
  using Type = TEntity;
};

template <typename TResult, typename TEntity>
struct MapEntity<TResult (*)(Q1Entity<TEntity> &) noexcept>
    : MapEntity<TResult (*)(Q1Entity<TEntity> &)> {};

template <typename, typename = void> struct HasMapEntity : std::false_type {};

template <typename T>
struct HasMapEntity<T, std::void_t<typename MapEntity<decltype(&T::ConfigureEntity)>::Type>>
    : std::true_type {
};
} // namespace Q1Detail

// Optional compatibility helper; maps do not need to inherit from this type.
template <typename TEntity> struct Q1EntityMap {
  using EntityType = TEntity;
};

class Q1ORM_EXPORT Q1ModelBuilder {
public:
  explicit Q1ModelBuilder(Q1Context *context = nullptr) : m_context(context) {}
  Q1ModelBuilder(const Q1ModelBuilder &) = delete;
  Q1ModelBuilder &operator=(const Q1ModelBuilder &) = delete;

  // Configure the actual context member; property offsets must stay valid.
  // Passing the member also registers it and needs no EntityType alias.
  template <typename TMap, typename TEntity>
  Q1ModelBuilder &ApplyMap(Q1Entity<TEntity> &entity) {
    if (m_tables.contains(entity.GetTablePtr()))
      return *this;
    if (m_context)
      m_context->RegisterEntity(&entity);

    TMap::ConfigureEntity(entity);
    AddTable(entity.GetTablePtr());
    if constexpr (requires { TMap::CreateRelations(entity); }) {
      m_deferred.append([this, &entity] {
        for (const Q1Relation &relation : TMap::CreateRelations(entity))
          AddRelation(relation);
      });
    }
    return *this;
  }

  template <typename TMap> Q1ModelBuilder &ApplyMap() {
    static_assert(
        Q1Detail::HasMapEntity<TMap>::value,
        "Map must expose a public static ConfigureEntity(Q1Entity<Entity>&). "
        "Alternatively, use ApplyMap<Map>(entity).");
    if constexpr (Q1Detail::HasMapEntity<TMap>::value) {
      using TEntity = typename Q1Detail::MapEntity<decltype(&TMap::ConfigureEntity)>::Type;
      auto *entity = m_context ? m_context->ResolveEntity<TEntity>() : nullptr;
      if (entity)
        return ApplyMap<TMap>(*entity);
      m_errors << QStringLiteral(
                      "ApplyMap: entity %1 is not registered. "
                      "Use ApplyMap<Map>(entity) or RegisterEntity(&entity).")
                      .arg(QString::fromLatin1(typeid(TEntity).name()));
    }
    return *this;
  }

  Q1ModelBuilder &AddRelation(const Q1Relation &relation);
  bool Build();

  const QList<Q1Table *> &Tables() const { return m_tables; }
  const QList<Q1Relation> &Relations() const { return m_relations; }
  const QStringList &Errors() const { return m_errors; }
  bool HasErrors() const { return !m_errors.isEmpty(); }

  static QString FkKey(const Q1Relation &relation);

private:
  void AddTable(Q1Table *table);
  void Validate();

  Q1Context *m_context = nullptr;
  QList<Q1Table *> m_tables;
  QList<Q1Relation> m_relations;
  QSet<QString> m_relationKeys;
  QList<std::function<void()>> m_deferred;
  QStringList m_errors;
};

#endif // Q1MODELBUILDER_H
