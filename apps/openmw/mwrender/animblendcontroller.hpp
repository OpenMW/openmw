#ifndef OPENMW_MWRENDER_ANIMBLENDCONTROLLER_H
#define OPENMW_MWRENDER_ANIMBLENDCONTROLLER_H

#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include <osgAnimation/Bone>

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
    class AnimBlendController : public SceneUtil::NodeCallback<AnimBlendController<NodeClass>, NodeClass*>,
                                public SceneUtil::Controller
    {
    public:
        AnimBlendController(osg::ref_ptr<SceneUtil::KeyframeController> keyframeTrack,
            const AnimBlendStateData& animState, osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules);

        AnimBlendController() {}

        AnimBlendController(const AnimBlendController& other, const osg::CopyOp&)
            : AnimBlendController(other.mKeyframeTrack, other.mAnimState, other.mAnimBlendRules)
        {
        }

        META_Object(MWRender, AnimBlendController<NodeClass>)

        void operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv);
        void operator()(osgAnimation::Bone* node, osg::NodeVisitor* nv);

        void setKeyframeTrack(osg::ref_ptr<SceneUtil::KeyframeController> kft, const AnimBlendStateData& animState,
            osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules);

        osg::Callback* getAsCallback() { return this; }

        bool getBlendTrigger() const { return mBlendTrigger; }

        void gatherRecursiveBoneTransforms(osgAnimation::Bone* parent, bool isRoot = true);
        void applyBoneBlend(osgAnimation::Bone* parent);

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
        osg::ref_ptr<SceneUtil::KeyframeController> mKeyframeTrack;

        std::unordered_map<osg::Node*, osg::Matrixf> mBlendBoneTransforms;

        inline void calculateInterpFactor(float time);
    };

    using NifAnimBlendController = AnimBlendController<NifOsg::MatrixTransform>;
    using BoneAnimBlendController = AnimBlendController<osgAnimation::Bone>;

    // Assigned to child bones with an instance of AnimBlendController
    class BoneAnimBlendControllerWrapper : public osg::Callback
    {
    public:
        BoneAnimBlendControllerWrapper(osg::ref_ptr<BoneAnimBlendController> rootCallback, osgAnimation::Bone* node)
            : mRootCallback(rootCallback)
            , mNode(node)
        {
        }

        BoneAnimBlendControllerWrapper() {}

        BoneAnimBlendControllerWrapper(const BoneAnimBlendControllerWrapper& copy, const osg::CopyOp&)
            : mRootCallback(copy.mRootCallback)
            , mNode(copy.mNode)
        {
        }

        META_Object(MWRender, BoneAnimBlendControllerWrapper)

        bool run(osg::Object* object, osg::Object* data) override
        {
            mRootCallback->applyBoneBlend(mNode);
            traverse(object, data);
            return true;
        }

    private:
        osg::ref_ptr<BoneAnimBlendController> mRootCallback;
        osgAnimation::Bone* mNode;
    };
}

#endif
