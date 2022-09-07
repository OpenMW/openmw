#ifndef OPENMW_COMPONENTS_MISC_META_H
#define OPENMW_COMPONENTS_MISC_META_H

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace Misc
{
    template <class T, class Tuple>
    struct TupleHasType;

    template <class T, class... Args>
    struct TupleHasType<T, std::tuple<Args...>>
    {
        static constexpr bool value = (std::is_same_v<T, Args> || ...);
    };

    template <class T, class Tuple>
    struct TupleTypeIndex;

    template <class T, class... Types>
    struct TupleTypeIndex<T, std::tuple<T, Types...>>
    {
        static constexpr std::size_t value = 0;
    };

    template <class T, class U, class... Types>
    struct TupleTypeIndex<T, std::tuple<U, Types...>>
    {
        static constexpr std::size_t value = 1 + TupleTypeIndex<T, std::tuple<Types...>>::value;
    };
}

#endif
