#ifndef OPENMW_COMPONENTS_MISC_TYPETRAITS_H
#define OPENMW_COMPONENTS_MISC_TYPETRAITS_H

#include <optional>
#include <type_traits>

namespace Misc
{
    template <class T>
    struct IsOptional : std::false_type {};

    template <class T>
    struct IsOptional<std::optional<T>> : std::true_type {};

    template <class T>
    inline constexpr bool isOptional = IsOptional<T>::value;
}

#endif
