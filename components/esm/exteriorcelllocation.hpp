#ifndef OPENMW_COMPONENTS_ESM_EXTERIORCELLLOCATION_H
#define OPENMW_COMPONENTS_ESM_EXTERIORCELLLOCATION_H

#include "refid.hpp"

#include <components/esm3/loadcell.hpp>

#include <ostream>
#include <tuple>

namespace ESM
{
    struct ExteriorCellLocation
    {
        int mX = 0;
        int mY = 0;
        ESM::RefId mWorldspace = ESM::Cell::sDefaultWorldspaceId;

        ExteriorCellLocation() = default;

        ExteriorCellLocation(int x, int y, ESM::RefId worldspace)
            : mX(x)
            , mY(y)
            , mWorldspace(worldspace)
        {
        }

        friend bool operator==(const ExteriorCellLocation& lhs, const ExteriorCellLocation& rhs) = default;

        friend inline bool operator<(const ExteriorCellLocation& lhs, const ExteriorCellLocation& rhs)
        {
            return std::make_tuple(lhs.mX, lhs.mY, lhs.mWorldspace) < std::make_tuple(rhs.mX, rhs.mY, rhs.mWorldspace);
        }

        friend inline std::ostream& operator<<(std::ostream& stream, const ExteriorCellLocation& value)
        {
            return stream << "{" << value.mX << ", " << value.mY << ", " << value.mWorldspace << "}";
        }
    };
}

namespace std
{
    template <>
    struct hash<ESM::ExteriorCellLocation>
    {
        std::size_t operator()(const ESM::ExteriorCellLocation& toHash) const
        {
            // Compute individual hash values for first,
            // second and third and combine them using XOR
            // and bit shifting:

            return ((hash<int>()(toHash.mX) ^ (hash<int>()(toHash.mY) << 1)) >> 1)
                ^ (hash<ESM::RefId>()(toHash.mWorldspace) << 1);
        }
    };
}

#endif
