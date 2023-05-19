#ifndef OPENMW_ESM_UTIL_H
#define OPENMW_ESM_UTIL_H
#include <cmath>

#include <osg/Quat>
#include <osg/Vec2>
#include <osg/Vec2i>
#include <osg/Vec3f>

#include <components/esm/refid.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/misc/constants.hpp>

namespace ESM
{

    // format 0, savegames only

    struct Quaternion
    {
        float mValues[4];

        Quaternion() = default;

        Quaternion(const osg::Quat& q)
        {
            mValues[0] = q.w();
            mValues[1] = q.x();
            mValues[2] = q.y();
            mValues[3] = q.z();
        }

        operator osg::Quat() const { return osg::Quat(mValues[1], mValues[2], mValues[3], mValues[0]); }
    };

    struct Vector3
    {
        float mValues[3];

        Vector3() = default;

        Vector3(const osg::Vec3f& v)
        {
            mValues[0] = v.x();
            mValues[1] = v.y();
            mValues[2] = v.z();
        }

        operator osg::Vec3f() const { return osg::Vec3f(mValues[0], mValues[1], mValues[2]); }
    };

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

        bool operator==(const ExteriorCellLocation& other) const
        {
            return mX == other.mX && mY == other.mY && mWorldspace == other.mWorldspace;
        }

        bool operator<(const ExteriorCellLocation& other) const
        {
            return std::make_tuple(mX, mY, mWorldspace) < std::make_tuple(other.mX, other.mY, other.mWorldspace);
        }
    };

    inline bool isEsm4Ext(ESM::RefId worldspaceId)
    {
        return worldspaceId != ESM::Cell::sDefaultWorldspaceId;
    }

    inline int getCellSize(ESM::RefId worldspaceId)
    {

        return isEsm4Ext(worldspaceId) ? Constants::ESM4CellSizeInUnits : Constants::CellSizeInUnits;
    }

    // Vertex count of a side of a land record
    int getLandSize(ESM::RefId worldspaceId);

    inline ESM::ExteriorCellLocation positionToExteriorCellLocation(
        float x, float y, ESM::RefId worldspaceId = ESM::Cell::sDefaultWorldspaceId)
    {
        const float cellSize = getCellSize(worldspaceId);
        return { static_cast<int>(std::floor(x / cellSize)), static_cast<int>(std::floor(y / cellSize)), worldspaceId };
    }

    osg::Vec2 indexToPosition(const ESM::ExteriorCellLocation& cellIndex, bool centre = false);
    ///< Convert cell numbers to position.
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
