#ifndef OPENMW_COMPONENTS_ESM_VEC2IREFID_HPP
#define OPENMW_COMPONENTS_ESM_VEC2IREFID_HPP

#include <functional>
#include <iosfwd>

#include <utility>

namespace ESM
{
    class Vec2iRefId
    {
    public:
        constexpr Vec2iRefId() = default;

        constexpr explicit Vec2iRefId(std::pair<int32_t, int32_t> value) noexcept
            : mValue(value)
        {
        }

        std::pair<int32_t, int32_t> getValue() const { return mValue; }

        std::string toString() const;

        std::string toDebugString() const;

        constexpr bool operator==(Vec2iRefId rhs) const noexcept { return mValue == rhs.mValue; }

        constexpr bool operator<(Vec2iRefId rhs) const noexcept { return mValue < rhs.mValue; }

        friend std::ostream& operator<<(std::ostream& stream, Vec2iRefId value);

        friend struct std::hash<Vec2iRefId>;

    private:
        std::pair<int32_t, int32_t> mValue = std::pair<int32_t, int32_t>(0, 0);
    };
}

namespace std
{
    template <>
    struct hash<ESM::Vec2iRefId>
    {
        std::size_t operator()(ESM::Vec2iRefId value) const noexcept
        {
            return (53 + std::hash<int32_t>{}(value.mValue.first)) * 53 + std::hash<int32_t>{}(value.mValue.second);
        }
    };
}

#endif
