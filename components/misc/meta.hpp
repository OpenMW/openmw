#ifndef OPENMW_COMPONENTS_MISC_META_H
#define OPENMW_COMPONENTS_MISC_META_H

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
}

#endif
