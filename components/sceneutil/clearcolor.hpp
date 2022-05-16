#ifndef OPENMW_COMPONENTS_SCENEUTIL_CLEARCOLOR_H
#define OPENMW_COMPONENTS_SCENEUTIL_CLEARCOLOR_H

#include <osg/StateAttribute>
#include <osg/Vec4f>

namespace SceneUtil
{
    class ClearColor : public osg::StateAttribute
    {
    public:
        ClearColor() : mMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) {}
        ClearColor(const osg::Vec4f& color, GLbitfield mask) : mColor(color), mMask(mask) {}

        ClearColor(const ClearColor& copy,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
            : osg::StateAttribute(copy,copyop), mColor(copy.mColor), mMask(copy.mMask) {}

        META_StateAttribute(fx, ClearColor, static_cast<osg::StateAttribute::Type>(100))

        int compare(const StateAttribute& sa) const override
        {
            COMPARE_StateAttribute_Types(ClearColor, sa);

            COMPARE_StateAttribute_Parameter(mColor);
            COMPARE_StateAttribute_Parameter(mMask);

            return 0;
        }

        void apply(osg::State& state) const override
        {
            glClearColor(mColor[0], mColor[1], mColor[2], mColor[3]);
            glClear(mMask);
        }

    private:
        osg::Vec4f mColor;
        GLbitfield mMask;
    };
}

#endif
