#ifndef OPENMW_COMPONENTS_SCENEUTIL_LIGHTCONTROLLER_H
#define OPENMW_COMPONENTS_SCENEUTIL_LIGHTCONTROLLER_H

#include <osg/NodeCallback>
#include <osg/Vec4f>

namespace SceneUtil
{

    /// @brief Controller class to handle a pulsing and/or flickering light
    /// @note Must be set on a SceneUtil::LightSource.
    class LightController : public osg::NodeCallback
    {
    public:
        enum LightType {
            LT_Normal,
            LT_Flicker,
            LT_FlickerSlow,
            LT_Pulse,
            LT_PulseSlow
        };

        LightController();

        void setType(LightType type);

        void setDiffuse(const osg::Vec4f& color);

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    private:
        LightType mType;
        osg::Vec4f mDiffuseColor;
        float mPhase;
        float mBrightness;
        double mStartTime;
        double mLastTime;
        float mTicksToAdvance;
    };

}

#endif
