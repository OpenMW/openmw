#include <components/sceneutil/osgacontroller.hpp>

#include <osg/MatrixTransform>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/ref_ptr>

#include <osgAnimation/Animation>
#include <osgAnimation/Channel>
#include <osgAnimation/Sampler>
#include <osgAnimation/UpdateMatrixTransform>

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/resource/animation.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/keyframe.hpp>

namespace SceneUtil
{
    LinkVisitor::LinkVisitor()
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
    {
        mAnimation = nullptr;
    }

    void LinkVisitor::link(osgAnimation::UpdateMatrixTransform* umt)
    {
        const osgAnimation::ChannelList& channels = mAnimation->getChannels();
        for (const auto& channel : channels)
        {
            const std::string& channelName = channel->getName();
            const std::string& channelTargetName = channel->getTargetName();

            if (channelTargetName != umt->getName())
                continue;

            // check if we can link a StackedTransformElement to the current Channel
            for (const auto& stackedTransform : umt->getStackedTransforms())
            {
                osgAnimation::StackedTransformElement* element = stackedTransform.get();
                if (element && !element->getName().empty() && channelName == element->getName())
                {
                    osgAnimation::Target* target = element->getOrCreateTarget();
                    if (target)
                    {
                        channel->setTarget(target);
                    }
                }
            }
        }
    }

    void LinkVisitor::setAnimation(Resource::Animation* animation)
    {
        mAnimation = animation;
    }

    void LinkVisitor::apply(osg::Node& node)
    {
        osg::Callback* cb = node.getUpdateCallback();
        while (cb)
        {
            osgAnimation::UpdateMatrixTransform* umt = dynamic_cast<osgAnimation::UpdateMatrixTransform*>(cb);
            if (umt)
                if (Misc::StringUtils::lowerCase(node.getName()) != "bip01")
                    link(umt);
            cb = cb->getNestedCallback();
        }

        if (node.getNumChildrenRequiringUpdateTraversal())
            traverse(node);
    }

    OsgAnimationController::OsgAnimationController(const OsgAnimationController& copy, const osg::CopyOp& copyop)
        : osg::Object(copy, copyop)
        , SceneUtil::KeyframeController(copy)
        , SceneUtil::NodeCallback<OsgAnimationController>(copy, copyop)
        , mEmulatedAnimations(copy.mEmulatedAnimations)
    {
        mLinker = nullptr;
        for (const auto& mergedAnimationTrack : copy.mMergedAnimationTracks)
        {
            Resource::Animation* copiedAnimationTrack
                = static_cast<Resource::Animation*>(mergedAnimationTrack.get()->clone(copyop));
            mMergedAnimationTracks.emplace_back(copiedAnimationTrack);
        }
    }

    osg::Matrixf OsgAnimationController::getTransformForNode(float time, const std::string_view name) const
    {
        std::string animationName;
        float newTime = time;

        // Find the correct animation based on time
        for (const EmulatedAnimation& emulatedAnimation : mEmulatedAnimations)
        {
            if (time >= emulatedAnimation.mStartTime && time <= emulatedAnimation.mStopTime)
            {
                newTime = time - emulatedAnimation.mStartTime;
                animationName = emulatedAnimation.mName;
                break;
            }
        }

        // Find the bone's transform track in animation
        for (const auto& mergedAnimationTrack : mMergedAnimationTracks)
        {
            if (mergedAnimationTrack->getName() != animationName)
                continue;

            const osgAnimation::ChannelList& channels = mergedAnimationTrack->getChannels();

            for (const auto& channel : channels)
            {
                if (!Misc::StringUtils::ciEqual(name, channel->getTargetName()) || channel->getName() != "transform")
                    continue;

                if (osgAnimation::MatrixLinearSampler* templateSampler
                    = dynamic_cast<osgAnimation::MatrixLinearSampler*>(channel->getSampler()))
                {
                    osg::Matrixf matrix;
                    templateSampler->getValueAt(newTime, matrix);
                    return matrix;
                }
            }
        }

        return osg::Matrixf::identity();
    }

    osg::Vec3f OsgAnimationController::getTranslation(float time) const
    {
        return getTransformForNode(time, "bip01").getTrans();
    }

    void OsgAnimationController::update(float time, const std::string& animationName)
    {
        for (const auto& mergedAnimationTrack : mMergedAnimationTracks)
        {
            if (mergedAnimationTrack->getName() == animationName)
                mergedAnimationTrack->update(time);
        }
    }

    void OsgAnimationController::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (hasInput())
        {
            if (mNeedToLink)
            {
                for (const auto& mergedAnimationTrack : mMergedAnimationTracks)
                {
                    if (!mLinker.valid())
                        mLinker = new LinkVisitor();
                    mLinker->setAnimation(mergedAnimationTrack);
                    node->accept(*mLinker);
                }
                mNeedToLink = false;
            }

            float time = getInputValue(nv);

            for (const EmulatedAnimation& emulatedAnimation : mEmulatedAnimations)
            {
                if (time > emulatedAnimation.mStartTime && time < emulatedAnimation.mStopTime)
                {
                    update(time - emulatedAnimation.mStartTime, emulatedAnimation.mName);
                }
            }

            // Reset the transform of this node to whats in the animation
            // we force this here because downstream some code relies on the bone having a non-modified transform
            // as this is how the NIF controller behaves. RotationController is a good example of this.
            // Without this here, it causes osgAnimation skeletons to spin wildly
            static_cast<osg::MatrixTransform*>(node)->setMatrix(getTransformForNode(time, node->getName()));
        }

        traverse(node, nv);
    }

    void OsgAnimationController::setEmulatedAnimations(const std::vector<EmulatedAnimation>& emulatedAnimations)
    {
        mEmulatedAnimations = emulatedAnimations;
    }

    void OsgAnimationController::addMergedAnimationTrack(osg::ref_ptr<Resource::Animation> animationTrack)
    {
        mMergedAnimationTracks.emplace_back(animationTrack);
    }
}
