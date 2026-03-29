//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_TYPE_TRAITS_HPP
#define DELTABASE_TYPE_TRAITS_HPP
#include <variant>

namespace misc
{
    template <typename T, typename Variant> struct is_in_variant;

    template <typename T, typename... Ts>
    struct is_in_variant<T, std::variant<Ts...>> : std::disjunction<std::is_same<T, Ts>...>
    {
    };

    template <typename T, typename Variant>
    inline constexpr bool is_in_variant_v = is_in_variant<T, Variant>::value;

}

#endif // DELTABASE_TYPE_TRAITS_HPP
