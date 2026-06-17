#ifndef OPENMW_COMPONENTS_SCENEUTIL_CLUSTEREDLIGHTING_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_CLUSTEREDLIGHTING_HPP

#include <cstdint>
#include <osg/Vec4f>

namespace SceneUtil
{
    struct Cluster
    {
        osg::Vec4f mMinPoint;
        osg::Vec4f mMaxPoint;
    };

    struct LightGrid
    {
        std::uint32_t mOffset;
        std::uint32_t mCount;
    };

    struct PointLight
    {
        osg::Vec4f mPosition;
        osg::Vec4f mDiffuse;
        osg::Vec4f mAmbient;
        osg::Vec4f mSpecular;
        float mConstant;
        float mLinear;
        float mQuadratic;
        float mRadius;
    };
}

#endif
