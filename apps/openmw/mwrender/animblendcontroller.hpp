#ifndef OPENMW_MWRENDER_ANIMBLENDCONTROLLER_H
#define OPENMW_MWRENDER_ANIMBLENDCONTROLLER_H

#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include <osgAnimation/Bone>

#include <components/debug/debuglog.hpp>
#include <components/nifosg/matrixtransform.hpp>
#include <components/sceneutil/animblendrules.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/sceneutil/nodecallback.hpp>

namespace MWRender
{
    namespace Easings
    {
        typedef float (*EasingFn)(float);
    }

    struct AnimBlendStateData
    {
        std::string mGroupname;
        std::string mStartKey;
    };

    template <typename NodeClass>
    class AnimBlendControllerBase : public SceneUtil::NodeCallback<AnimBlendControllerBase<NodeClass>, NodeClass*>,
                                    public SceneUtil::Controller
    {
    public:
        AnimBlendControllerBase(osg::ref_ptr<SceneUtil::KeyframeController> keyframeTrack, AnimBlendStateData animState,
            osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules);

        void operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv);
        void operator()(osgAnimation::Bone* node, osg::NodeVisitor* nv);

        void setKeyframeTrack(osg::ref_ptr<SceneUtil::KeyframeController> kft, AnimBlendStateData animState,
            osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules);

        osg::Callback* getAsCallback() { return this; }

        bool getBlendTrigger() const { return mBlendTrigger; }

        void gatherRecursiveBoneTransforms(osgAnimation::Bone* parent, bool isRoot = true);
        void applyBoneBlend(osgAnimation::Bone* parent);

        const char* libraryName() const override { return "MWRender"; }
        const char* className() const override { return "AnimBlendController"; }

    protected:
        osg::ref_ptr<SceneUtil::KeyframeController> mKeyframeTrack;

        inline void calculateInterpFactor(float time);

    private:
        Easings::EasingFn mEasingFn;
        float mBlendDuration;

        bool mBlendTrigger = false;
        float mBlendStartTime;
        osg::Quat mBlendStartRot;
        osg::Vec3f mBlendStartTrans;
        float mBlendStartScale;

        float mTimeFactor;
        float mInterpFactor;
        bool mInterpActive;

        AnimBlendStateData mAnimState;
        osg::ref_ptr<const SceneUtil::AnimBlendRules> mAnimBlendRules;

        std::unordered_map<osg::Node*, osg::Matrixf> mBlendBoneTransforms;
    };

    using AnimBlendController = AnimBlendControllerBase<NifOsg::MatrixTransform>;
    using BoneAnimBlendController = AnimBlendControllerBase<osgAnimation::Bone>;

    // Assigned to child bones with an instance of AnimBlendControllerBase
    class BoneAnimBlendControllerWrapper : public osg::Callback
    {
    public:
        BoneAnimBlendControllerWrapper(osg::ref_ptr<BoneAnimBlendController> rootCallback, osg::Node* node)
        {
            mRootCallback = rootCallback;
            mNode = dynamic_cast<osgAnimation::Bone*>(node);
        }

        bool run(osg::Object* object, osg::Object* data) override
        {
            mRootCallback->applyBoneBlend(mNode);
            traverse(object, data);
            return true;
        }

        const char* libraryName() const override { return "openmw"; }
        const char* className() const override { return "AnimBlendController"; }

    private:
        osg::ref_ptr<BoneAnimBlendController> mRootCallback;
        osgAnimation::Bone* mNode;
    };
}

#endif
