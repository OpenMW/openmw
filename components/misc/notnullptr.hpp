#ifndef OPENMW_COMPONENTS_MISC_NOTNULLPTR_H
#define OPENMW_COMPONENTS_MISC_NOTNULLPTR_H

#include <cassert>
#include <cstddef>
#include <type_traits>

namespace Misc
{
    template <class T>
    class NotNullPtr
    {
    public:
        constexpr NotNullPtr(T* value) noexcept
            : mValue(value)
        {
            assert(mValue != nullptr);
        }

        NotNullPtr(std::nullptr_t) = delete;

        constexpr operator T*() const noexcept { return mValue; }

        constexpr T* operator->() const noexcept { return mValue; }

        constexpr T& operator*() const noexcept { return *mValue; }

    private:
        T* mValue;
    };
}

#endif
