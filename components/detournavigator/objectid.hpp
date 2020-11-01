#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OBJECTID_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OBJECTID_H

#include <cstddef>
#include <unordered_map>

namespace DetourNavigator
{
    class ObjectId
    {
    public:
        template <class T>
        explicit ObjectId(T* value) noexcept
            : mValue(reinterpret_cast<std::size_t>(value))
        {
        }

        std::size_t value() const noexcept
        {
            return mValue;
        }

        friend bool operator <(const ObjectId lhs, const ObjectId rhs) noexcept
        {
            return lhs.mValue < rhs.mValue;
        }

        friend bool operator ==(const ObjectId lhs, const ObjectId rhs) noexcept
        {
            return lhs.mValue == rhs.mValue;
        }

    private:
        std::size_t mValue;
    };
}

namespace std
{
    template <>
    struct hash<DetourNavigator::ObjectId>
    {
        std::size_t operator ()(const DetourNavigator::ObjectId value) const noexcept
        {
            return value.value();
        }
    };
}

#endif
