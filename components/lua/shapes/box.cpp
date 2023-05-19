#include "box.hpp"

namespace LuaUtil
{
    Box::Box(const osg::Vec3f& center, const osg::Vec3f& halfSize, const osg::Quat& rotation)
        : mCenter(center)
        , mHalfSize(halfSize)
        , mRotation(rotation)
    {
    }

    Box::Box(const osg::Matrix& transform)
    {
        osg::Quat _;
        transform.decompose(mCenter, mRotation, mHalfSize, _);
    }

    std::array<osg::Vec3f, 8> Box::vertices() const
    {
        return {
            mCenter + mRotation * osg::Vec3f(-mHalfSize.x(), -mHalfSize.y(), -mHalfSize.z()),
            mCenter + mRotation * osg::Vec3f(mHalfSize.x(), -mHalfSize.y(), -mHalfSize.z()),
            mCenter + mRotation * osg::Vec3f(mHalfSize.x(), mHalfSize.y(), -mHalfSize.z()),
            mCenter + mRotation * osg::Vec3f(-mHalfSize.x(), mHalfSize.y(), -mHalfSize.z()),
            mCenter + mRotation * osg::Vec3f(-mHalfSize.x(), -mHalfSize.y(), mHalfSize.z()),
            mCenter + mRotation * osg::Vec3f(mHalfSize.x(), -mHalfSize.y(), mHalfSize.z()),
            mCenter + mRotation * osg::Vec3f(mHalfSize.x(), mHalfSize.y(), mHalfSize.z()),
            mCenter + mRotation * osg::Vec3f(-mHalfSize.x(), mHalfSize.y(), mHalfSize.z()),
        };
    }

    osg::Matrix Box::asTransform() const
    {
        osg::Matrix transform;
        transform.preMultTranslate(mCenter);
        transform.preMultRotate(mRotation);
        transform.preMultScale(mHalfSize);
        return transform;
    }

    bool Box::operator==(const Box& other) const
    {
        return mCenter == other.mCenter && mHalfSize == other.mHalfSize && mRotation == other.mRotation;
    }
}