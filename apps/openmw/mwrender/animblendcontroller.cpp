#include "animblendcontroller.hpp"
#include "rotatecontroller.hpp"

#include <components/debug/debuglog.hpp>

#include <osgAnimation/Bone>

#include <cassert>
#include <string>
#include <vector>

namespace MWRender
{
    namespace
    {
        // Animation Easing/Blending functions
        namespace Easings
        {
            float linear(float x)
            {
                return x;
            }

            float sineOut(float x)
            {
                return std::sin((x * osg::PIf) / 2.f);
            }

            float sineIn(float x)
            {
                return 1.f - std::cos((x * osg::PIf) / 2.f);
            }

            float sineInOut(float x)
            {
                return -(std::cos(osg::PIf * x) - 1.f) / 2.f;
            }

            float cubicOut(float t)
            {
                float t1 = 1.f - t;
                return 1.f - (t1 * t1 * t1); // (1-t)^3
            }

            float cubicIn(float x)
            {
                return x * x * x; // x^3
            }

            float cubicInOut(float x)
            {
                if (x < 0.5f)
                {
                    return 4.f * x * x * x; // 4x^3
                }
                else
                {
                    float x2 = -2.f * x + 2.f;
                    return 1.f - (x2 * x2 * x2) / 2.f; // (1 - (-2x + 2)^3)/2
                }
            }

            float quartOut(float t)
            {
                float t1 = 1.f - t;
                return 1.f - (t1 * t1 * t1 * t1); // (1-t)^4
            }

            float quartIn(float t)
            {
                return t * t * t * t; // t^4
            }

            float quartInOut(float x)
            {
                if (x < 0.5f)
                {
                    return 8.f * x * x * x * x; // 8x^4
                }
                else
                {
                    float x2 = -2.f * x + 2.f;
                    return 1.f - (x2 * x2 * x2 * x2) / 2.f; // 1 - ((-2x + 2)^4)/2
                }
            }

            float springOutGeneric(float x, float lambda)
            {
                // Higher lambda = lower swing amplitude. 1 = 150% swing amplitude.
                // w is the frequency of oscillation in the easing func, controls the amount of overswing
                const float w = 1.5f * osg::PIf; // 4.71238
                return 1.f - expf(-lambda * x) * std::cos(w * x);
            }

            float springOutWeak(float x)
            {
                return springOutGeneric(x, 4.f);
            }

            float springOutMed(float x)
            {
                return springOutGeneric(x, 3.f);
            }

            float springOutStrong(float x)
            {
                return springOutGeneric(x, 2.f);
            }

            float springOutTooMuch(float x)
            {
                return springOutGeneric(x, 1.f);
            }

            const std::unordered_map<std::string, EasingFn> easingsMap = {
                { "linear", Easings::linear },
                { "sineOut", Easings::sineOut },
                { "sineIn", Easings::sineIn },
                { "sineInOut", Easings::sineInOut },
                { "cubicOut", Easings::cubicOut },
                { "cubicIn", Easings::cubicIn },
                { "cubicInOut", Easings::cubicInOut },
                { "quartOut", Easings::quartOut },
                { "quartIn", Easings::quartIn },
                { "quartInOut", Easings::quartInOut },
                { "springOutWeak", Easings::springOutWeak },
                { "springOutMed", Easings::springOutMed },
                { "springOutStrong", Easings::springOutStrong },
                { "springOutTooMuch", Easings::springOutTooMuch },
            };
        }

        osg::Vec3f vec3fLerp(float t, const osg::Vec3f& start, const osg::Vec3f& end)
        {
            return start + (end - start) * t;
        }
    }

    AnimBlendController::AnimBlendController(const osg::ref_ptr<SceneUtil::KeyframeController>& keyframeTrack,
        const AnimBlendStateData& newState, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules)
        : mEasingFn(&Easings::sineOut)
    {
        setKeyframeTrack(keyframeTrack, newState, blendRules);
    }

    AnimBlendController::AnimBlendController()
        : mEasingFn(&Easings::sineOut)
    {
    }

    NifAnimBlendController::NifAnimBlendController(const osg::ref_ptr<SceneUtil::KeyframeController>& keyframeTrack,
        const AnimBlendStateData& newState, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules)
        : AnimBlendController(keyframeTrack, newState, blendRules)
    {
    }

    BoneAnimBlendController::BoneAnimBlendController(const osg::ref_ptr<SceneUtil::KeyframeController>& keyframeTrack,
        const AnimBlendStateData& newState, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules)
        : AnimBlendController(keyframeTrack, newState, blendRules)
    {
    }

    void AnimBlendController::setKeyframeTrack(const osg::ref_ptr<SceneUtil::KeyframeController>& kft,
        const AnimBlendStateData& newState, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules)
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
                    if (const auto it = Easings::easingsMap.find(blendRule->mEasing); it != Easings::easingsMap.end())
                    {
                        mEasingFn = it->second;
                        mBlendDuration = blendRule->mDuration;
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

    void AnimBlendController::calculateInterpFactor(float time)
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

    void BoneAnimBlendController::gatherRecursiveBoneTransforms(osgAnimation::Bone* bone, bool isRoot)
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

    void BoneAnimBlendController::applyBoneBlend(osgAnimation::Bone* bone)
    {
        // If we are done with interpolation then we can safely skip this as the bones are correct
        if (!mInterpActive)
            return;

        // Shouldn't happen, but potentially an edge case where a new bone was added
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

    void BoneAnimBlendController::operator()(osgAnimation::Bone* node, osg::NodeVisitor* nv)
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

        SceneUtil::NodeCallback<BoneAnimBlendController, osgAnimation::Bone*>::traverse(node, nv);
    }

    void NifAnimBlendController::operator()(NifOsg::MatrixTransform* node, osg::NodeVisitor* nv)
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

            // Nif mRotationScale is used here because it's unaffected by the side-effects of RotationController
            mBlendStartRot = node->mRotationScale.toOsgMatrix().getRotate();
            mBlendStartTrans = node->getMatrix().getTrans();
            mBlendStartScale = node->mScale;

            // Subtract any rotate controller's offset from start transform (if it appears after this callback)
            // this is required otherwise the blend start will be with an offset, then offset could be applied again
            // fixes an issue with camera jumping during first person sneak jumping camera
            osg::Callback* updateCb = node->getUpdateCallback()->getNestedCallback();
            while (updateCb)
            {
                MWRender::RotateController* rotateController = dynamic_cast<MWRender::RotateController*>(updateCb);
                if (rotateController)
                {
                    const osg::Quat& rotate = rotateController->getRotate();
                    const osg::Vec3f& offset = rotateController->getOffset();

                    osg::NodePathList nodepaths = node->getParentalNodePaths(rotateController->getRelativeTo());
                    osg::Quat worldOrient;
                    if (!nodepaths.empty())
                    {
                        osg::Matrixf worldMat = osg::computeLocalToWorld(nodepaths[0]);
                        worldOrient = worldMat.getRotate();
                    }

                    worldOrient = worldOrient * rotate.inverse();
                    const osg::Quat worldOrientInverse = worldOrient.inverse();

                    mBlendStartTrans -= worldOrientInverse * offset;
                }

                updateCb = updateCb->getNestedCallback();
            }
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

        SceneUtil::NodeCallback<NifAnimBlendController, NifOsg::MatrixTransform*>::traverse(node, nv);
    }
}
