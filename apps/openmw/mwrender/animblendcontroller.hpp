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

    class AnimBlendController : public SceneUtil::Controller
    {
    public:
        AnimBlendController(const osg::ref_ptr<SceneUtil::KeyframeController>& keyframeTrack,
            const AnimBlendStateData& animState, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules);

        AnimBlendController() {}

        void setKeyframeTrack(const osg::ref_ptr<SceneUtil::KeyframeController>& kft,
            const AnimBlendStateData& animState, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules);

        bool getBlendTrigger() const { return mBlendTrigger; }

    protected:
        Easings::EasingFn mEasingFn;
        float mBlendDuration = 0.0f;
        float mBlendStartTime = 0.0f;
        float mTimeFactor = 0.0f;
        float mInterpFactor = 0.0f;

        bool mBlendTrigger = false;
        bool mInterpActive = false;

        AnimBlendStateData mAnimState;
        osg::ref_ptr<const SceneUtil::AnimBlendRules> mAnimBlendRules;
        osg::ref_ptr<SceneUtil::KeyframeController> mKeyframeTrack;

        std::unordered_map<osg::Node*, osg::Matrixf> mBlendBoneTransforms;

        inline void calculateInterpFactor(float time);
    };

    class NifAnimBlendController : public SceneUtil::NodeCallback<NifAnimBlendController, NifOsg::MatrixTransform*>,
                                   public AnimBlendController
    {
    public:
        NifAnimBlendController(const osg::ref_ptr<SceneUtil::KeyframeController>& keyframeTrack,
            const AnimBlendStateData& animState, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules);

        NifAnimBlendController() {}

        NifAnimBlendController(const NifAnimBlendController& other, const osg::CopyOp&)
            : NifAnimBlendController(other.mKeyframeTrack, other.mAnimState, other.mAnimBlendRules)
        {
        }

        META_Object(MWRender, NifAnimBlendController)

        void operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv);

        osg::Callback* getAsCallback() { return this; }

    private:
        osg::Quat mBlendStartRot;
        osg::Vec3f mBlendStartTrans;
        float mBlendStartScale = 0.0f;
    };

    class BoneAnimBlendController : public SceneUtil::NodeCallback<BoneAnimBlendController, osgAnimation::Bone*>,
                                    public AnimBlendController
    {
    public:
        BoneAnimBlendController(const osg::ref_ptr<SceneUtil::KeyframeController>& keyframeTrack,
            const AnimBlendStateData& animState, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules);

        BoneAnimBlendController() {}

        BoneAnimBlendController(const BoneAnimBlendController& other, const osg::CopyOp&)
            : BoneAnimBlendController(other.mKeyframeTrack, other.mAnimState, other.mAnimBlendRules)
        {
        }

        void gatherRecursiveBoneTransforms(osgAnimation::Bone* parent, bool isRoot = true);
        void applyBoneBlend(osgAnimation::Bone* parent);

        META_Object(MWRender, BoneAnimBlendController)

        void operator()(osgAnimation::Bone* node, osg::NodeVisitor* nv);

        osg::Callback* getAsCallback() { return this; }
    };

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
