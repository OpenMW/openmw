#ifndef OPENMW_COMPONENTS_ESM4_TYPETRAITS
#define OPENMW_COMPONENTS_ESM4_TYPETRAITS

#include <type_traits>

namespace ESM4
{
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
    struct HasFullName : std::false_type
    {
    };

    template <class T>
    struct HasFullName<T, std::void_t<decltype(T::mFullName)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasFullName = HasFullName<T>::value;

    template <class T, class = std::void_t<>>
    struct HasCellFlags : std::false_type
    {
    };

    template <class T>
    struct HasCellFlags<T, std::void_t<decltype(T::mCellFlags)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasCellFlags = HasCellFlags<T>::value;

    template <class T, class = std::void_t<>>
    struct HasX : std::false_type
    {
    };

    template <class T>
    struct HasX<T, std::void_t<decltype(T::mX)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasX = HasX<T>::value;

    template <class T, class = std::void_t<>>
    struct HasY : std::false_type
    {
    };

    template <class T>
    struct HasY<T, std::void_t<decltype(T::mY)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasY = HasY<T>::value;

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

    template <class T, class = std::void_t<>>
    struct HasData : std::false_type
    {
    };

    template <class T>
    struct HasData<T, std::void_t<decltype(T::mData)>> : std::true_type
    {
    };

    template <class T>
    inline constexpr bool hasData = HasData<T>::value;
}

#endif // OPENMW_COMPONENTS_ESM4_TYPETRAITS
