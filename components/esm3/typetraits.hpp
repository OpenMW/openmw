#ifndef OPENMW_COMPONENTS_ESM3_TYPETRAITS
#define OPENMW_COMPONENTS_ESM3_TYPETRAITS

#include <type_traits>

namespace ESM
{
    template <class T, class = std::void_t<>>
    struct HasIndex : std::false_type
    {
    };

    template <class T>
    struct HasIndex<T, std::void_t<decltype(T::mIndex)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasIndex = HasIndex<T>::value;
}

#endif // OPENMW_COMPONENTS_ESM3_TYPETRAITS
