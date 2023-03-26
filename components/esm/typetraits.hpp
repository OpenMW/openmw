#ifndef OPENMW_COMPONENTS_ESM_TYPETRAITS
#define OPENMW_COMPONENTS_ESM_TYPETRAITS

#include <type_traits>

namespace ESM
{
    template <class T, class = std::void_t<>>
    struct HasId : std::false_type
    {
    };

    template <class T>
    struct HasId<T, std::void_t<decltype(T::mId)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasId = HasId<T>::value;

    template <class T, class = std::void_t<>>
    struct HasModel : std::false_type
    {
    };

    template <class T>
    struct HasModel<T, std::void_t<decltype(T::mModel)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasModel = HasModel<T>::value;
}

#endif // OPENMW_COMPONENTS_ESM_TYPETRAITS
