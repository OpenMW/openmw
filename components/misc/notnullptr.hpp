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
        NotNullPtr(T* value)
            : mValue(value)
        {
            assert(mValue != nullptr);
        }

        NotNullPtr(std::nullptr_t) = delete;

        operator T*() const { return mValue; }

        T* operator->() const { return mValue; }

        T& operator*() const { return *mValue; }

    private:
        T* mValue;
    };
}

#endif
