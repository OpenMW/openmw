#include <components/sceneutil/osgacontroller.hpp>

#include <osg/Geode>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/ref_ptr>
#include <osg/StateSet>

#include <osgAnimation/Animation>
#include <osgAnimation/AnimationUpdateCallback>
#include <osgAnimation/Channel>
#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/Bone>
#include <osgAnimation/Sampler>
#include <osgAnimation/Skeleton>
#include <osgAnimation/RigGeometry>
#include <osgAnimation/UpdateMatrixTransform>

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>
#include <components/resource/animation.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/keyframe.hpp>

namespace SceneUtil
{
    LinkVisitor::LinkVisitor() : osg::NodeVisitor( TRAVERSE_ALL_CHILDREN )
    {
        mAnimation = nullptr;
    }

    void LinkVisitor::link(osgAnimation::UpdateMatrixTransform* umt)
    {
        const osgAnimation::ChannelList& channels = mAnimation->getChannels();
        for (const auto& channel: channels)
        {
            const std::string& channelName = channel->getName();
            const std::string& channelTargetName = channel->getTargetName();

            if (channelTargetName != umt->getName()) continue;

            // check if we can link a StackedTransformElement to the current Channel
            for (auto stackedTransform : umt->getStackedTransforms())
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

    void LinkVisitor::handle_stateset(osg::StateSet* stateset)
    {
        if (!stateset)
            return;
        const osg::StateSet::AttributeList& attributeList = stateset->getAttributeList();
        for (auto attribute : attributeList)
        {
            osg::StateAttribute* sattr = attribute.second.first.get();
            osgAnimation::UpdateMatrixTransform* umt = dynamic_cast<osgAnimation::UpdateMatrixTransform*>(sattr->getUpdateCallback()); //Can this even be in sa?
            if (umt) link(umt);
        }
    }

    void LinkVisitor::setAnimation(Resource::Animation* animation)
    {
        mAnimation = animation;
    }

    void LinkVisitor::apply(osg::Node& node)
    {
        osg::StateSet* st = node.getStateSet();
        if (st)
        handle_stateset(st);

        osg::Callback* cb = node.getUpdateCallback();
        while (cb)
        {
            osgAnimation::UpdateMatrixTransform* umt = dynamic_cast<osgAnimation::UpdateMatrixTransform*>(cb);
            if (umt)
                if (Misc::StringUtils::lowerCase(node.getName()) != "bip01") link(umt);
            cb = cb->getNestedCallback();
        }

        traverse( node );
    }

    void LinkVisitor::apply(osg::Geode& node)
    {
        for (unsigned int i = 0; i < node.getNumDrawables(); i++)
        {
            osg::Drawable* drawable = node.getDrawable(i);
            if (drawable && drawable->getStateSet())
                handle_stateset(drawable->getStateSet());
        }
        apply(static_cast<osg::Node&>(node));
    }

    OsgAnimationController::OsgAnimationController(const OsgAnimationController &copy, const osg::CopyOp &copyop) : SceneUtil::KeyframeController(copy, copyop)
    , mEmulatedAnimations(copy.mEmulatedAnimations)
    {
        mLinker = nullptr;
        for (const auto& mergedAnimationTrack : copy.mMergedAnimationTracks)
        {
            Resource::Animation* copiedAnimationTrack = static_cast<Resource::Animation*>(mergedAnimationTrack.get()->clone(copyop));
            mMergedAnimationTracks.emplace_back(copiedAnimationTrack);
        }
    }

    osg::Vec3f OsgAnimationController::getTranslation(float time) const
    {
        osg::Vec3f translationValue;
        std::string animationName;
        float newTime = time;

        //Find the correct animation based on time
        for (const EmulatedAnimation& emulatedAnimation : mEmulatedAnimations)
        {
            if (time >= emulatedAnimation.mStartTime && time <= emulatedAnimation.mStopTime)
            {
                newTime = time - emulatedAnimation.mStartTime;
                animationName = emulatedAnimation.mName;
            }
        }

        //Find the root transform track in animation
        for (const auto& mergedAnimationTrack : mMergedAnimationTracks)
        {
            if (mergedAnimationTrack->getName() != animationName) continue;

            const osgAnimation::ChannelList& channels = mergedAnimationTrack->getChannels();

            for (const auto& channel: channels)
            {
                if (channel->getTargetName() != "bip01" || channel->getName() != "transform") continue;

                if ( osgAnimation::MatrixLinearSampler* templateSampler = dynamic_cast<osgAnimation::MatrixLinearSampler*> (channel->getSampler()) )
                {
                    osg::Matrixf matrix;
                    templateSampler->getValueAt(newTime, matrix);
                    translationValue = matrix.getTrans();
                    return osg::Vec3f(translationValue[0], translationValue[1], translationValue[2]);
                }
            }
        }

        return osg::Vec3f();
    }

    void OsgAnimationController::update(float time, const std::string& animationName)
    {
        for (const auto& mergedAnimationTrack : mMergedAnimationTracks)
        {
            if (mergedAnimationTrack->getName() == animationName) mergedAnimationTrack->update(time);
        }
    }

    void OsgAnimationController::operator() (osg::Node* node, osg::NodeVisitor* nv)
    {
        if (hasInput())
        {
            if (mNeedToLink)
            {
                for (const auto& mergedAnimationTrack : mMergedAnimationTracks)
                {
                    if (!mLinker.valid()) mLinker = new LinkVisitor();
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
