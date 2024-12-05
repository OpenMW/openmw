#ifndef OPENMW_ESM3_POSITION_H
#define OPENMW_ESM3_POSITION_H

#include <components/misc/concepts.hpp>
#include <osg/Vec3f>
#include <tuple>

namespace ESM
{
    // Position and rotation
    struct Position
    {
        float pos[3]{};

        // In radians
        float rot[3]{};

        osg::Vec3f asVec3() const { return osg::Vec3f(pos[0], pos[1], pos[2]); }

        osg::Vec3f asRotationVec3() const { return osg::Vec3f(rot[0], rot[1], rot[2]); }

        friend inline bool operator<(const Position& l, const Position& r)
        {
            const auto tuple = [](const Position& v) { return std::tuple(v.asVec3(), v.asRotationVec3()); };
            return tuple(l) < tuple(r);
        }
    };

    bool inline operator==(const Position& left, const Position& right) noexcept
    {
        return left.pos[0] == right.pos[0] && left.pos[1] == right.pos[1] && left.pos[2] == right.pos[2]
            && left.rot[0] == right.rot[0] && left.rot[1] == right.rot[1] && left.rot[2] == right.rot[2];
    }

    bool inline operator!=(const Position& left, const Position& right) noexcept
    {
        return left.pos[0] != right.pos[0] || left.pos[1] != right.pos[1] || left.pos[2] != right.pos[2]
            || left.rot[0] != right.rot[0] || left.rot[1] != right.rot[1] || left.rot[2] != right.rot[2];
    }

    template <Misc::SameAsWithoutCvref<Position> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.pos, v.rot);
    }
}
#endif