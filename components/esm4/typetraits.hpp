#ifndef OPENMW_COMPONENTS_ESM4_TYPETRAITS
#define OPENMW_COMPONENTS_ESM4_TYPETRAITS

#include <type_traits>

namespace ESM4
{
    template <class T, class = std::void_t<>>
    struct HasFormId : std::false_type
    {
    };

    template <class T>
    struct HasFormId<T, std::void_t<decltype(T::mFormId)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasFormId = HasFormId<T>::value;

    template <class T, class = std::void_t<>>
    struct HasParentFormId : std::false_type
    {
    };

    template <class T>
    struct HasParentFormId<T, std::void_t<decltype(T::mParentFormId)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasParentFormId = HasParentFormId<T>::value;

    template <class T, class = std::void_t<>>
    struct HasParent : std::false_type
    {
    };

    template <class T>
    struct HasParent<T, std::void_t<decltype(T::mParent)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasParent = HasParent<T>::value;

    template <class T, class = std::void_t<>>
    struct HasFlags : std::false_type
    {
    };

    template <class T>
    struct HasFlags<T, std::void_t<decltype(T::mFlags)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasFlags = HasFlags<T>::value;

    template <class T, class = std::void_t<>>
    struct HasEditorId : std::false_type
    {
    };

    template <class T>
    struct HasEditorId<T, std::void_t<decltype(T::mEditorId)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasEditorId = HasEditorId<T>::value;

    template <class T, class = std::void_t<>>
    struct HasNif : std::false_type
    {
    };

    template <class T>
    struct HasNif<T, std::void_t<decltype(T::mNif)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasNif = HasNif<T>::value;

    template <class T, class = std::void_t<>>
    struct HasKf : std::false_type
    {
    };

    template <class T>
    struct HasKf<T, std::void_t<decltype(T::mKf)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasKf = HasKf<T>::value;

    template <class T, class = std::void_t<>>
    struct HasType : std::false_type
    {
    };

    template <class T>
    struct HasType<T, std::void_t<decltype(T::mType)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasType = HasType<T>::value;

    template <class T, class = std::void_t<>>
    struct HasValue : std::false_type
    {
    };

    template <class T>
    struct HasValue<T, std::void_t<decltype(T::mValue)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasValue = HasValue<T>::value;
}

#endif // OPENMW_COMPONENTS_ESM4_TYPETRAITS
