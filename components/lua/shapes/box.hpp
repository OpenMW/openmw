#ifndef COMPONENTS_LUA_SHAPES_BOX_H
#define COMPONENTS_LUA_SHAPES_BOX_H

#include <array>

#include <osg/Matrix>
#include <osg/Quat>
#include <osg/Vec3f>

namespace LuaUtil
{
    class Box
    {
    public:
        Box(const osg::Vec3f& center, const osg::Vec3f& halfSize, const osg::Quat& rotation = osg::Quat());
        Box(const osg::Matrix& transform);

        std::array<osg::Vec3f, 8> vertices() const;

        osg::Matrix asTransform() const;

        // TODO: Add `contains` and `intersects` methods

        bool operator==(const Box& other) const;

        osg::Vec3f mCenter;
        osg::Vec3f mHalfSize;
        osg::Quat mRotation;
    };
}
#endif // COMPONENTS_LUA_SHAPES_BOX_H
