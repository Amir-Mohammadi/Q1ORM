#ifndef Q1MAPPINGNAMES_H
#define Q1MAPPINGNAMES_H

#include <QString>
#include <string_view>
#include <type_traits>

namespace Q1Detail {
// C++20 has no standard member-name reflection. Keep compiler-specific
// signature parsing isolated here; explicit column names remain supported.
template <auto Member> constexpr std::string_view MemberNameView()
{
    static_assert(std::is_member_object_pointer_v<decltype(Member)>,
                  "Mapping requires a pointer to a data member.");
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view signature = __PRETTY_FUNCTION__;
    constexpr auto start = signature.find("Member = ") + 9;
    constexpr auto end = signature.find_first_of(";]", start);
#elif defined(_MSC_VER)
    constexpr std::string_view signature = __FUNCSIG__;
    constexpr auto start = signature.find("MemberNameView<") + 15;
    constexpr auto end = signature.rfind(">(void)");
#else
    static_assert(sizeof(decltype(Member)) == 0,
                  "Automatic mapping names require GCC, Clang, or MSVC.");
    return {};
#endif
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
    constexpr auto qualified = signature.substr(start, end - start);
    constexpr auto separator = qualified.rfind("::");
    static_assert(separator != std::string_view::npos,
                  "Cannot infer member name; supply an explicit column name.");
    return qualified.substr(separator + 2);
#endif
}

template <auto Member> QString MemberName()
{
    constexpr auto name = MemberNameView<Member>();
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

template <typename T> QString EntityName()
{
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view signature = __PRETTY_FUNCTION__;
    constexpr auto start = signature.find("T = ") + 4;
    constexpr auto end = signature.find_first_of(";]", start);
#elif defined(_MSC_VER)
    constexpr std::string_view signature = __FUNCSIG__;
    constexpr auto start = signature.find("EntityName<") + 11;
    constexpr auto end = signature.rfind(">(void)");
#else
    return {}; // Other compilers can use ToTableName explicitly.
#endif
#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)
    auto name = signature.substr(start, end - start);
    if (name.starts_with("struct ")) name.remove_prefix(7);
    if (name.starts_with("class ")) name.remove_prefix(6);
    // Use the unqualified type name, without English pluralization.
    const auto separator = name.rfind("::");
    if (separator != std::string_view::npos) name.remove_prefix(separator + 2);
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
#endif
}
} // namespace Q1Detail

#endif // Q1MAPPINGNAMES_H
