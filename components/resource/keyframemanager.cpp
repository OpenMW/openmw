#include "keyframemanager.hpp"

#include <components/vfs/manager.hpp>

#include <osgAnimation/Animation>
#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/Channel>

#include <components/nifosg/nifloader.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/sceneutil/osgacontroller.hpp>
#include <components/misc/stringops.hpp>

#include "animation.hpp"
#include "objectcache.hpp"
#include "scenemanager.hpp"

namespace Resource
{

    RetrieveAnimationsVisitor::RetrieveAnimationsVisitor(SceneUtil::KeyframeHolder& target, osg::ref_ptr<osgAnimation::BasicAnimationManager> animationManager,
        const std::string& normalized, const VFS::Manager* vfs) :
        osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), mTarget(target), mAnimationManager(animationManager), mNormalized(normalized), mVFS(vfs) {}

    void RetrieveAnimationsVisitor::apply(osg::Node& node)
    {
        if (node.libraryName() == std::string("osgAnimation") && node.className() == std::string("Bone") && Misc::StringUtils::lowerCase(node.getName()) == std::string("bip01"))
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
                        mergedAnimationTrack->setName(animationName);

                        const osgAnimation::ChannelList& channels = animation->getChannels();
                        for (const auto& channel: channels)
                        {
                            mergedAnimationTrack->addChannel(channel.get()->clone()); // is ->clone needed?
                        }

                        callback->addMergedAnimationTrack(mergedAnimationTrack);

                        float startTime = animation->getStartTime();
                        float stopTime = startTime + animation->getDuration();

                        SceneUtil::EmulatedAnimation emulatedAnimation;
                        emulatedAnimation.mStartTime = startTime;
                        emulatedAnimation.mStopTime = stopTime;
                        emulatedAnimation.mName = animationName;
                        emulatedAnimations.emplace_back(emulatedAnimation);
                    }
                }

                // mTextKeys is a nif-thing, used by OpenMW's animation system
                // Format is likely "AnimationName: [Keyword_optional] [Start OR Stop]"
                // AnimationNames are keywords like idle2, idle3... AiPackages and various mechanics control which animations are played
                // Keywords can be stuff like Loop, Equip, Unequip, Block, InventoryHandtoHand, InventoryWeaponOneHand, PickProbe, Slash, Thrust, Chop... even "Slash Small Follow"
                // osgAnimation formats should have a .txt file with the same name, each line holding a textkey and whitespace separated time value
                // e.g. idle: start 0.0333
                try
                {
                    Files::IStreamPtr textKeysFile = mVFS->get(changeFileExtension(mNormalized, "txt"));
                    std::string line;
                    while ( getline (*textKeysFile, line) )
                    {
                        mTarget.mTextKeys.emplace(parseTimeSignature(line), parseTextKey(line));
                    }
                }
                catch (std::exception&)
                {
                    Log(Debug::Warning) << "No textkey file found for " << mNormalized;
                }

                callback->setEmulatedAnimations(emulatedAnimations);
                mTarget.mKeyframeControllers.emplace(node.getName(), callback);
            }

        traverse(node);
    }

    std::string RetrieveAnimationsVisitor::parseTextKey(const std::string& line)
    {
        size_t spacePos = line.find_last_of(' ');
        if (spacePos != std::string::npos)
             return line.substr(0, spacePos);
        return "";
    }

    double RetrieveAnimationsVisitor::parseTimeSignature(const std::string& line)
    {
        size_t spacePos = line.find_last_of(' ');
        double time = 0.0;
        if (spacePos != std::string::npos && spacePos + 1 < line.size())
             time = std::stod(line.substr(spacePos + 1));
        return time;
    }

    std::string RetrieveAnimationsVisitor::changeFileExtension(const std::string& file, const std::string& ext)
    {
        size_t extPos = file.find_last_of('.');
        if (extPos != std::string::npos && extPos+1 < file.size())
        {
            return file.substr(0, extPos + 1) + ext;
        }
        return file;
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
                    Resource::RetrieveAnimationsVisitor rav(*loaded.get(), bam, normalized, mVFS);
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
