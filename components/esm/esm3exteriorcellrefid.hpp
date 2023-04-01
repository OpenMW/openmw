#ifndef OPENMW_COMPONENTS_ESM_ESM3EXTERIORCELLREFID_HPP
#define OPENMW_COMPONENTS_ESM_ESM3EXTERIORCELLREFID_HPP

#include <functional>
#include <iosfwd>

#include <utility>

namespace ESM
{
    class ESM3ExteriorCellRefId
    {
    public:
        constexpr ESM3ExteriorCellRefId() = default;

        constexpr explicit ESM3ExteriorCellRefId(int32_t x, int32_t y) noexcept
            : mX(x)
            , mY(y)
        {
        }

        std::string toString() const;

        std::string toDebugString() const;

        int32_t getX() const { return mX; }
        int32_t getY() const { return mY; }

        constexpr bool operator==(ESM3ExteriorCellRefId rhs) const noexcept { return mX == rhs.mX && mY == rhs.mY; }

        constexpr bool operator<(ESM3ExteriorCellRefId rhs) const noexcept { return mX < rhs.mX && mY < rhs.mY; }

        friend std::ostream& operator<<(std::ostream& stream, ESM3ExteriorCellRefId value);

        friend struct std::hash<ESM3ExteriorCellRefId>;

    private:
        int32_t mX = 0;
        int32_t mY = 0;
    };
}

namespace std
{
    template <>
    struct hash<ESM::ESM3ExteriorCellRefId>
    {
        std::size_t operator()(ESM::ESM3ExteriorCellRefId value) const noexcept
        {
            return (53 + std::hash<int32_t>{}(value.mX)) * 53 + std::hash<int32_t>{}(value.mY);
        }
    };
}

#endif
