#include "keyframemanager.hpp"

#include <components/vfs/manager.hpp>

#include <osgAnimation/Animation>
#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/Channel>

#include <components/nifosg/nifloader.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/sceneutil/osgacontroller.hpp>

#include "animation.hpp"
#include "objectcache.hpp"
#include "scenemanager.hpp"

namespace Resource
{

    RetrieveAnimationsVisitor::RetrieveAnimationsVisitor(SceneUtil::KeyframeHolder& target, osg::ref_ptr<osgAnimation::BasicAnimationManager> animationManager) : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), mTarget(target), mAnimationManager(animationManager) {}

    void RetrieveAnimationsVisitor::apply(osg::Node& node)
    {
        if (node.libraryName() == std::string("osgAnimation") && node.className() == std::string("Bone") && node.getName() == std::string("bip01"))
            {
                osg::ref_ptr<SceneUtil::OsgAnimationController> callback = new SceneUtil::OsgAnimationController();

                std::vector<SceneUtil::EmulatedAnimation> emulatedAnimations;

                for (auto animation : mAnimationManager->getAnimationList())
                {
                    if (animation)
                    {
                        if (animation->getName() == "Default") //"Default" is osg dae plugin's default naming scheme for unnamed animations
                        {
                            animation->setName(std::string("idle")); // animation naming scheme "idle: start" and "idle: stop" is the default idle animation that OpenMW seems to want to play
                        }

                        osg::ref_ptr<Resource::Animation> mergedAnimationTrack = new Resource::Animation;
                        std::string animationName = animation->getName();
                        std::string start = animationName + std::string(": start");
                        std::string stop = animationName + std::string(": stop");

                        const osgAnimation::ChannelList& channels = animation->getChannels();
                        for (const auto& channel: channels)
                        {
                            mergedAnimationTrack->addChannel(channel.get()->clone()); // is ->clone needed?
                        }
                        mergedAnimationTrack->setName(animation->getName());
                        callback->addMergedAnimationTrack(mergedAnimationTrack);

                        float startTime = animation->getStartTime();
                        float stopTime = startTime + animation->getDuration();

                        // mTextKeys is a nif-thing, used by OpenMW's animation system
                        // Format is likely "AnimationName: [Keyword_optional] [Start OR Stop]"
                        // AnimationNames are keywords like idle2, idle3... AiPackages and various mechanics control which animations are played
                        // Keywords can be stuff like Loop, Equip, Unequip, Block, InventoryHandtoHand, InventoryWeaponOneHand, PickProbe, Slash, Thrust, Chop... even "Slash Small Follow"
                        mTarget.mTextKeys.emplace(startTime, std::move(start));
                        mTarget.mTextKeys.emplace(stopTime, std::move(stop));

                        SceneUtil::EmulatedAnimation emulatedAnimation;
                        emulatedAnimation.mStartTime = startTime;
                        emulatedAnimation.mStopTime = stopTime;
                        emulatedAnimation.mName = animationName;
                        emulatedAnimations.emplace_back(emulatedAnimation);
                    }
                }
                callback->setEmulatedAnimations(emulatedAnimations);
                mTarget.mKeyframeControllers.emplace(node.getName(), callback);
            }

        traverse(node);
    }
}

namespace Resource
{

    KeyframeManager::KeyframeManager(const VFS::Manager* vfs, SceneManager* sceneManager)
        : ResourceManager(vfs)
        , mSceneManager(sceneManager)
    {
    }

    KeyframeManager::~KeyframeManager()
    {
    }

    osg::ref_ptr<const SceneUtil::KeyframeHolder> KeyframeManager::get(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
        if (obj)
            return osg::ref_ptr<const SceneUtil::KeyframeHolder>(static_cast<SceneUtil::KeyframeHolder*>(obj.get()));
        else
        {
            osg::ref_ptr<SceneUtil::KeyframeHolder> loaded (new SceneUtil::KeyframeHolder);
            std::string ext = Resource::getFileExtension(normalized);
            if (ext == "kf")
            {
                NifOsg::Loader::loadKf(Nif::NIFFilePtr(new Nif::NIFFile(mVFS->getNormalized(normalized), normalized)), *loaded.get());
            }
            else
            {
                osg::ref_ptr<osg::Node> scene = const_cast<osg::Node*> ( mSceneManager->getTemplate(normalized).get() );
                osg::ref_ptr<osgAnimation::BasicAnimationManager> bam = dynamic_cast<osgAnimation::BasicAnimationManager*> (scene->getUpdateCallback());
                if (bam)
                {
                    Resource::RetrieveAnimationsVisitor rav(*loaded.get(), bam);
                    scene->accept(rav);
                }
            }
            mCache->addEntryToObjectCache(normalized, loaded);
            return loaded;
        }
    }

    void KeyframeManager::reportStats(unsigned int frameNumber, osg::Stats *stats) const
    {
        stats->setAttribute(frameNumber, "Keyframe", mCache->getCacheSize());
    }



}
