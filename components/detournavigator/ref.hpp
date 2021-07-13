#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_REF_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_REF_H

#include <algorithm>
#include <cassert>

namespace DetourNavigator
{
    template <typename T>
    struct Ref
    {
        T& mRef;

        constexpr explicit Ref(T& ref) noexcept : mRef(ref) {}

        friend bool operator==(const Ref& lhs, const Ref& rhs)
        {
            return lhs.mRef == rhs.mRef;
        }
    };

    template <typename T, std::size_t size>
    struct ArrayRef
    {
        T (&mRef)[size];

        constexpr explicit ArrayRef(T (&ref)[size]) noexcept : mRef(ref) {}

        friend bool operator==(const ArrayRef& lhs, const ArrayRef& rhs)
        {
            return std::equal(std::begin(lhs.mRef), std::end(lhs.mRef), std::begin(rhs.mRef));
        }
    };

    template <typename T>
    struct Span
    {
        T* mBegin;
        T* mEnd;

        constexpr explicit Span(T* data, int size) noexcept
            : mBegin(data)
            , mEnd(data + static_cast<std::size_t>(size))
        {}

        friend bool operator==(const Span& lhs, const Span& rhs)
        {
            // size is already equal if headers are equal
            assert((lhs.mEnd - lhs.mBegin) == (rhs.mEnd - rhs.mBegin));
            return std::equal(lhs.mBegin, lhs.mEnd, rhs.mBegin);
        }
    };
}

#endif
