#include "animblendcontroller.hpp"

#include <components/debug/debuglog.hpp>

#include <osgAnimation/Bone>

#include <string>
#include <vector>

namespace MWRender
{
    /// Animation Easing/Blending functions
    namespace Easings
    {
        float linear(float x)
        {
            return x;
        }
        float sineOut(float x)
        {
            return sin((x * 3.14) / 2);
        }
        float sineIn(float x)
        {
            return 1 - cos((x * 3.14) / 2);
        }
        float sineInOut(float x)
        {
            return -(cos(3.14 * x) - 1) / 2;
        }
        float cubicOut(float t)
        {
            return 1 - powf(1 - t, 3);
        }
        float cubicIn(float x)
        {
            return powf(x, 3);
        }
        float cubicInOut(float x)
        {
            return x < 0.5 ? 4 * x * x * x : 1 - powf(-2 * x + 2, 3) / 2;
        }
        float quartOut(float t)
        {
            return 1 - powf(1 - t, 4);
        }
        float quartIn(float t)
        {
            return powf(t, 4);
        }
        float quartInOut(float x)
        {
            return x < 0.5 ? 8 * x * x * x * x : 1 - powf(-2 * x + 2, 4) / 2;
        }
        float springOutGeneric(float x, float lambda, float w)
        {
            // Higher lambda = lower swing amplitude. 1 = 150% swing amplitude.
            // W corresponds to the amount of overswings, more = more. 4.71 = 1 overswing, 7.82 = 2
            return 1 - expf(-lambda * x) * cos(w * x);
        }
        float springOutWeak(float x)
        {
            return springOutGeneric(x, 4, 4.71);
        }
        float springOutMed(float x)
        {
            return springOutGeneric(x, 3, 4.71);
        }
        float springOutStrong(float x)
        {
            return springOutGeneric(x, 2, 4.71);
        }
        float springOutTooMuch(float x)
        {
            return springOutGeneric(x, 1, 4.71);
        }
        std::unordered_map<std::string, EasingFn> easingsMap = { { "linear", Easings::linear },
            { "sineOut", Easings::sineOut }, { "sineIn", Easings::sineIn }, { "sineInOut", Easings::sineInOut },
            { "cubicOut", Easings::cubicOut }, { "cubicIn", Easings::cubicIn }, { "cubicInOut", Easings::cubicInOut },
            { "quartOut", Easings::quartOut }, { "quartIn", Easings::quartIn }, { "quartInOut", Easings::quartInOut },
            { "springOutWeak", Easings::springOutWeak }, { "springOutMed", Easings::springOutMed },
            { "springOutStrong", Easings::springOutStrong }, { "springOutTooMuch", Easings::springOutTooMuch } };
    }

    namespace
    {
        osg::Vec3f vec3fLerp(float t, const osg::Vec3f& A, const osg::Vec3f& B)
        {
            return A + (B - A) * t;
        }
    }

    template <typename NodeClass>
    AnimBlendController<NodeClass>::AnimBlendController(osg::ref_ptr<SceneUtil::KeyframeController> keyframeTrack,
        const AnimBlendStateData& newState, osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules)
        : mTimeFactor(0.0f)
        , mInterpFactor(0.0f)
    {
        setKeyframeTrack(keyframeTrack, newState, blendRules);
    }

    template <typename NodeClass>
    void AnimBlendController<NodeClass>::setKeyframeTrack(osg::ref_ptr<SceneUtil::KeyframeController> kft,
        const AnimBlendStateData& newState, osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules)
    {
        // If animation has changed then start blending
        if (newState.mGroupname != mAnimState.mGroupname || newState.mStartKey != mAnimState.mStartKey
            || kft != mKeyframeTrack)
        {
            // Default blend settings
            mBlendDuration = 0;
            mEasingFn = &Easings::sineOut;

            if (blendRules)
            {
                // Finds a matching blend rule either in this or previous ruleset
                auto blendRule = blendRules->findBlendingRule(
                    mAnimState.mGroupname, mAnimState.mStartKey, newState.mGroupname, newState.mStartKey);

                if (blendRule)
                {
                    if (Easings::easingsMap.contains(blendRule->mEasing))
                    {
                        mBlendDuration = blendRule->mDuration;
                        mEasingFn = Easings::easingsMap[blendRule->mEasing];
                    }
                    else
                    {
                        Log(Debug::Warning)
                            << "Warning: animation blending rule contains invalid easing type: " << blendRule->mEasing;
                    }
                }
            }

            mAnimBlendRules = blendRules;
            mKeyframeTrack = kft;
            mAnimState = newState;
            mBlendTrigger = true;
        }
    }

    template <typename NodeClass>
    void AnimBlendController<NodeClass>::gatherRecursiveBoneTransforms(osgAnimation::Bone* bone, bool isRoot)
    {
        // Incase group traversal encountered something that isnt a bone
        if (!bone)
            return;

        mBlendBoneTransforms[bone] = bone->getMatrix();

        osg::Group* group = bone->asGroup();
        if (group)
        {
            for (unsigned int i = 0; i < group->getNumChildren(); ++i)
                gatherRecursiveBoneTransforms(dynamic_cast<osgAnimation::Bone*>(group->getChild(i)), false);
        }
    }

    template <typename NodeClass>
    void AnimBlendController<NodeClass>::applyBoneBlend(osgAnimation::Bone* bone)
    {
        // If we are done with interpolation then we can safely skip this as the bones are correct
        if (!mInterpActive)
            return;

        // Shouldnt happen, but potentially an edge case where a new bone was added
        // between gatherRecursiveBoneTransforms and this update
        // currently OpenMW will never do this
        assert(mBlendBoneTransforms.find(bone) != mBlendBoneTransforms.end());

        // Every frame the osgAnimation controller updates this
        // so it is ok that we update it directly below
        const osg::Matrixf& currentSampledMatrix = bone->getMatrix();
        const osg::Matrixf& lastSampledMatrix = mBlendBoneTransforms.at(bone);

        const osg::Vec3f scale = currentSampledMatrix.getScale();
        const osg::Quat rotation = currentSampledMatrix.getRotate();
        const osg::Vec3f translation = currentSampledMatrix.getTrans();

        const osg::Quat blendRotation = lastSampledMatrix.getRotate();
        const osg::Vec3f blendTrans = lastSampledMatrix.getTrans();

        osg::Quat lerpedRot;
        lerpedRot.slerp(mInterpFactor, blendRotation, rotation);

        osg::Matrixf lerpedMatrix;
        lerpedMatrix.makeRotate(lerpedRot);
        lerpedMatrix.setTrans(vec3fLerp(mInterpFactor, blendTrans, translation));

        // Scale is not lerped based on the idea that it is much more likely that scale animation will be used to
        // instantly hide/show objects in which case the scale interpolation is undesirable.
        lerpedMatrix = osg::Matrixd::scale(scale) * lerpedMatrix;

        // Apply new blended matrix
        osgAnimation::Bone* boneParent = bone->getBoneParent();
        bone->setMatrix(lerpedMatrix);
        if (boneParent)
            bone->setMatrixInSkeletonSpace(lerpedMatrix * boneParent->getMatrixInSkeletonSpace());
        else
            bone->setMatrixInSkeletonSpace(lerpedMatrix);
    }

    template <typename NodeClass>
    void AnimBlendController<NodeClass>::calculateInterpFactor(float time)
    {
        if (mBlendDuration != 0)
            mTimeFactor = std::min((time - mBlendStartTime) / mBlendDuration, 1.0f);
        else
            mTimeFactor = 1;

        mInterpActive = mTimeFactor < 1.0;

        if (mInterpActive)
            mInterpFactor = mEasingFn(mTimeFactor);
        else
            mInterpFactor = 1.0f;
    }

    template <typename NodeClass>
    void AnimBlendController<NodeClass>::operator()(osgAnimation::Bone* node, osg::NodeVisitor* nv)
    {
        // HOW THIS WORKS: This callback method is called only for bones with attached keyframe controllers
        // such as bip01, bip01 spine1 etc. The child bones of these controllers have their own callback wrapper
        // which will call this instance's applyBoneBlend for each child bone. The order of update is important
        // as the blending calculations expect the bone's skeleton matrix to be at the sample point
        float time = nv->getFrameStamp()->getSimulationTime();
        assert(node != nullptr);

        if (mBlendTrigger)
        {
            mBlendTrigger = false;
            mBlendStartTime = time;
        }

        calculateInterpFactor(time);

        if (mInterpActive)
            applyBoneBlend(node);

        SceneUtil::NodeCallback<AnimBlendController<NodeClass>, osgAnimation::Bone*>::traverse(node, nv);
    }

    template <typename NodeClass>
    void AnimBlendController<NodeClass>::operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv)
    {
        // HOW THIS WORKS: The actual retrieval of the bone transformation based on animation is done by the
        // KeyframeController (mKeyframeTrack). The KeyframeController retreives time data (playback position) every
        // frame from controller's input (getInputValue(nv)) which is bound to an appropriate AnimationState time value
        // in Animation.cpp. Animation.cpp ultimately manages animation playback via updating AnimationState objects and
        // determines when and what should be playing.
        // This controller exploits KeyframeController to get transformations and upon animation change blends from
        // the last known position to the new animated one.

        auto [translation, rotation, scale] = mKeyframeTrack->getCurrentTransformation(nv);

        float time = nv->getFrameStamp()->getSimulationTime();

        if (mBlendTrigger)
        {
            mBlendTrigger = false;
            mBlendStartTime = time;
            // Nif mRotation is used here because it's unaffected by the side-effects of RotationController
            mBlendStartRot = node->mRotationScale.toOsgMatrix().getRotate();
            mBlendStartTrans = node->getMatrix().getTrans();
            mBlendStartScale = node->mScale;
        }

        calculateInterpFactor(time);

        if (mInterpActive)
        {
            if (rotation)
            {
                osg::Quat lerpedRot;
                lerpedRot.slerp(mInterpFactor, mBlendStartRot, *rotation);
                node->setRotation(lerpedRot);
            }
            else
            {
                // This is necessary to prevent first person animation glitching out
                node->setRotation(node->mRotationScale);
            }

            if (translation)
            {
                osg::Vec3f lerpedTrans = vec3fLerp(mInterpFactor, mBlendStartTrans, *translation);
                node->setTranslation(lerpedTrans);
            }
        }
        else
        {
            if (translation)
                node->setTranslation(*translation);

            if (rotation)
                node->setRotation(*rotation);
            else
                node->setRotation(node->mRotationScale);
        }

        if (scale)
            // Scale is not lerped based on the idea that it is much more likely that scale animation will be used to
            // instantly hide/show objects in which case the scale interpolation is undesirable.
            node->setScale(*scale);

        SceneUtil::NodeCallback<AnimBlendController<NodeClass>, NifOsg::MatrixTransform*>::traverse(node, nv);
    }
}
