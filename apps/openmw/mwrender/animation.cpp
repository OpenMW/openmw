#include "animation.hpp"

#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/character.hpp"

namespace MWRender
{

Animation::Animation(const MWWorld::Ptr &ptr)
    : mPtr(ptr)
    , mController(NULL)
    , mInsert(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mAccumulate(Ogre::Vector3::ZERO)
    , mLastPosition(0.0f)
    , mCurrentKeys(NULL)
    , mCurrentAnim(NULL)
    , mCurrentTime(0.0f)
    , mStopTime(0.0f)
    , mPlaying(false)
    , mLooping(false)
    , mAnimVelocity(0.0f)
    , mAnimSpeedMult(1.0f)
{
}

Animation::~Animation()
{
    if(mInsert)
    {
        Ogre::SceneManager *sceneMgr = mInsert->getCreator();
        for(size_t i = 0;i < mEntityList.mEntities.size();i++)
            sceneMgr->destroyEntity(mEntityList.mEntities[i]);
    }
    mEntityList.mEntities.clear();
    mEntityList.mSkelBase = NULL;
}


void Animation::setAnimationSources(const std::vector<std::string> &names)
{
    if(!mEntityList.mSkelBase)
        return;

    Ogre::SkeletonManager &skelMgr = Ogre::SkeletonManager::getSingleton();

    mCurrentAnim = NULL;
    mCurrentKeys = NULL;
    mAnimVelocity = 0.0f;
    mAccumRoot = NULL;
    mNonAccumRoot = NULL;
    mSkeletonSources.clear();

    std::vector<std::string>::const_iterator nameiter = names.begin();
    for(nameiter = names.begin();nameiter != names.end();nameiter++)
    {
        Ogre::SkeletonPtr skel = skelMgr.getByName(*nameiter);
        if(skel.isNull())
        {
            NifOgre::Loader::createSkeleton(*nameiter);
            skel = skelMgr.getByName(*nameiter);
            if(skel.isNull())
            {
                std::cerr<< "Failed to get skeleton source "<<*nameiter <<std::endl;
                continue;
            }
        }
        skel->touch();

        Ogre::Skeleton::BoneIterator boneiter = skel->getBoneIterator();
        while(boneiter.hasMoreElements())
        {
            Ogre::Bone *bone = boneiter.getNext();
            Ogre::UserObjectBindings &bindings = bone->getUserObjectBindings();
            const Ogre::Any &data = bindings.getUserAny(NifOgre::sTextKeyExtraDataID);
            if(data.isEmpty() || !Ogre::any_cast<bool>(data))
                continue;

            if(!mNonAccumRoot)
            {
                mAccumRoot = mInsert;
                mNonAccumRoot = mEntityList.mSkelBase->getSkeleton()->getBone(bone->getName());
            }

            mSkeletonSources.push_back(skel);
            for(int i = 0;i < skel->getNumAnimations();i++)
            {
                Ogre::Animation *anim = skel->getAnimation(i);
                const Ogre::Any &groupdata = bindings.getUserAny(std::string(NifOgre::sTextKeyExtraDataID)+
                                                                "@"+anim->getName());
                if(!groupdata.isEmpty())
                    mTextKeys[anim->getName()] = Ogre::any_cast<NifOgre::TextKeyMap>(groupdata);
            }

            break;
        }
    }
}

void Animation::createEntityList(Ogre::SceneNode *node, const std::string &model)
{
    mInsert = node->createChildSceneNode();
    assert(mInsert);

    mEntityList = NifOgre::Loader::createEntities(mInsert, model);
    if(mEntityList.mSkelBase)
    {
        Ogre::AnimationStateSet *aset = mEntityList.mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator asiter = aset->getAnimationStateIterator();
        while(asiter.hasMoreElements())
        {
            Ogre::AnimationState *state = asiter.getNext();
            state->setEnabled(false);
            state->setLoop(false);
        }

        // Set the bones as manually controlled since we're applying the
        // transformations manually (needed if we want to apply an animation
        // from one skeleton onto another).
        Ogre::SkeletonInstance *skelinst = mEntityList.mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);
    }
}


bool Animation::hasAnimation(const std::string &anim)
{
    for(std::vector<Ogre::SkeletonPtr>::const_iterator iter(mSkeletonSources.begin());iter != mSkeletonSources.end();iter++)
    {
        if((*iter)->hasAnimation(anim))
            return true;
    }
    return false;
}


void Animation::setController(MWMechanics::CharacterController *controller)
{
    mController = controller;
}


void Animation::setAccumulation(const Ogre::Vector3 &accum)
{
    mAccumulate = accum;
}

void Animation::setSpeed(float speed)
{
    mAnimSpeedMult = 1.0f;
    if(mAnimVelocity > 1.0f && speed > 0.0f)
        mAnimSpeedMult = speed / mAnimVelocity;
}

void Animation::setLooping(bool loop)
{
    mLooping = loop;
}

void Animation::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}


void Animation::calcAnimVelocity()
{
    const Ogre::NodeAnimationTrack *track = 0;

    Ogre::Animation::NodeTrackIterator trackiter = mCurrentAnim->getNodeTrackIterator();
    while(!track && trackiter.hasMoreElements())
    {
        const Ogre::NodeAnimationTrack *cur = trackiter.getNext();
        if(cur->getAssociatedNode()->getName() == mNonAccumRoot->getName())
            track = cur;
    }

    if(track && track->getNumKeyFrames() > 1)
    {
        float loopstarttime = 0.0f;
        float loopstoptime = mCurrentAnim->getLength();
        NifOgre::TextKeyMap::const_iterator keyiter = mCurrentKeys->begin();
        while(keyiter != mCurrentKeys->end())
        {
            if(keyiter->second == "loop start")
                loopstarttime = keyiter->first;
            else if(keyiter->second == "loop stop")
            {
                loopstoptime = keyiter->first;
                break;
            }
            keyiter++;
        }

        if(loopstoptime > loopstarttime)
        {
            Ogre::TransformKeyFrame startkf(0, loopstarttime);
            Ogre::TransformKeyFrame endkf(0, loopstoptime);

            track->getInterpolatedKeyFrame(mCurrentAnim->_getTimeIndex(loopstarttime), &startkf);
            track->getInterpolatedKeyFrame(mCurrentAnim->_getTimeIndex(loopstoptime), &endkf);

            mAnimVelocity = startkf.getTranslate().distance(endkf.getTranslate()) /
                            (loopstoptime-loopstarttime);
        }
    }
}

void Animation::applyAnimation(const Ogre::Animation *anim, float time, Ogre::SkeletonInstance *skel)
{
    Ogre::TimeIndex timeindex = anim->_getTimeIndex(time);
    Ogre::Animation::NodeTrackIterator tracks = anim->getNodeTrackIterator();
    while(tracks.hasMoreElements())
    {
        Ogre::NodeAnimationTrack *track = tracks.getNext();
        const Ogre::String &targetname = track->getAssociatedNode()->getName();
        if(!skel->hasBone(targetname))
            continue;
        Ogre::Bone *bone = skel->getBone(targetname);
        bone->setOrientation(Ogre::Quaternion::IDENTITY);
        bone->setPosition(Ogre::Vector3::ZERO);
        bone->setScale(Ogre::Vector3::UNIT_SCALE);
        track->applyToNode(bone, timeindex);
    }

    // HACK: Dirty the animation state set so that Ogre will apply the
    // transformations to entities this skeleton instance is shared with.
    mEntityList.mSkelBase->getAllAnimationStates()->_notifyDirty();
}

static void updateBoneTree(const Ogre::SkeletonInstance *skelsrc, Ogre::Bone *bone)
{
    if(skelsrc->hasBone(bone->getName()))
    {
        Ogre::Bone *srcbone = skelsrc->getBone(bone->getName());
        if(!srcbone->getParent() || !bone->getParent())
        {
            bone->setOrientation(srcbone->getOrientation());
            bone->setPosition(srcbone->getPosition());
            bone->setScale(srcbone->getScale());
        }
        else
        {
            bone->_setDerivedOrientation(srcbone->_getDerivedOrientation());
            bone->_setDerivedPosition(srcbone->_getDerivedPosition());
            bone->setScale(Ogre::Vector3::UNIT_SCALE);
        }
    }
    else
    {
        // No matching bone in the source. Make sure it stays properly offset
        // from its parent.
        bone->resetToInitialState();
    }

    Ogre::Node::ChildNodeIterator boneiter = bone->getChildIterator();
    while(boneiter.hasMoreElements())
        updateBoneTree(skelsrc, static_cast<Ogre::Bone*>(boneiter.getNext()));
}

void Animation::updateSkeletonInstance(const Ogre::SkeletonInstance *skelsrc, Ogre::SkeletonInstance *skel)
{
    Ogre::Skeleton::BoneIterator boneiter = skel->getRootBoneIterator();
    while(boneiter.hasMoreElements())
        updateBoneTree(skelsrc, boneiter.getNext());
}


Ogre::Vector3 Animation::updatePosition(float time)
{
    if(mLooping)
        mCurrentTime = std::fmod(std::max(time, 0.0f), mCurrentAnim->getLength());
    else
        mCurrentTime = std::min(mCurrentAnim->getLength(), std::max(time, 0.0f));
    applyAnimation(mCurrentAnim, mCurrentTime, mEntityList.mSkelBase->getSkeleton());

    Ogre::Vector3 posdiff = Ogre::Vector3::ZERO;
    if(mNonAccumRoot)
    {
        /* Get the non-accumulation root's difference from the last update. */
        posdiff = (mNonAccumRoot->getPosition() - mLastPosition) * mAccumulate;

        /* Translate the accumulation root back to compensate for the move. */
        mLastPosition += posdiff;
        mAccumRoot->setPosition(-mLastPosition);
    }
    return posdiff;
}

void Animation::reset(const std::string &start, const std::string &stop)
{
    mNextKey = mCurrentKeys->begin();

    while(mNextKey != mCurrentKeys->end() && mNextKey->second != start)
        mNextKey++;
    if(mNextKey != mCurrentKeys->end())
        mCurrentTime = mNextKey->first;
    else
    {
        mNextKey = mCurrentKeys->begin();
        mCurrentTime = 0.0f;
    }

    if(stop.length() > 0)
    {
        NifOgre::TextKeyMap::const_iterator stopKey = mNextKey;
        while(stopKey != mCurrentKeys->end() && stopKey->second != stop)
            stopKey++;
        if(stopKey != mCurrentKeys->end())
            mStopTime = stopKey->first;
        else
            mStopTime = mCurrentAnim->getLength();
    }

    if(mNonAccumRoot)
    {
        const Ogre::NodeAnimationTrack *track = 0;
        Ogre::Animation::NodeTrackIterator trackiter = mCurrentAnim->getNodeTrackIterator();
        while(!track && trackiter.hasMoreElements())
        {
            const Ogre::NodeAnimationTrack *cur = trackiter.getNext();
            if(cur->getAssociatedNode()->getName() == mNonAccumRoot->getName())
                track = cur;
        }

        if(track)
        {
            Ogre::TransformKeyFrame kf(0, mCurrentTime);
            track->getInterpolatedKeyFrame(mCurrentAnim->_getTimeIndex(mCurrentTime), &kf);
            mLastPosition = kf.getTranslate() * mAccumulate;
        }
    }
}


bool Animation::handleEvent(float time, const std::string &evt)
{
    if(evt == "start" || evt == "loop start")
    {
        /* Do nothing */
        return true;
    }

    if(evt.compare(0, 7, "sound: ") == 0)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        sndMgr->playSound3D(mPtr, evt.substr(7), 1.0f, 1.0f);
        return true;
    }
    if(evt.compare(0, 10, "soundgen: ") == 0)
    {
        // FIXME: Lookup the SoundGen (SNDG) for the specified sound that corresponds
        // to this actor type
        return true;
    }

    if(evt == "loop stop")
    {
        if(mLooping)
        {
            reset("loop start", "");
            if(mCurrentTime >= time)
                return false;
        }
        return true;
    }
    if(evt == "stop")
    {
        if(mLooping)
        {
            reset("loop start", "");
            if(mCurrentTime >= time)
                return false;
            return true;
        }
        // fall-through
    }
    if(mController)
        mController->markerEvent(time, evt);
    return true;
}


void Animation::play(const std::string &groupname, const std::string &start, const std::string &stop, bool loop)
{
    try {
        bool found = false;
        /* Look in reverse; last-inserted source has priority. */
        for(std::vector<Ogre::SkeletonPtr>::const_reverse_iterator iter(mSkeletonSources.rbegin());iter != mSkeletonSources.rend();iter++)
        {
            if((*iter)->hasAnimation(groupname))
            {
                mCurrentAnim = (*iter)->getAnimation(groupname);
                mCurrentKeys = &mTextKeys[groupname];
                mAnimVelocity = 0.0f;

                if(mNonAccumRoot)
                    calcAnimVelocity();

                found = true;
                break;
            }
        }
        if(!found)
            throw std::runtime_error("Failed to find animation "+groupname);

        reset(start, stop);
        setLooping(loop);
        mPlaying = true;
    }
    catch(std::exception &e) {
        std::cerr<< e.what() <<std::endl;
    }
}

Ogre::Vector3 Animation::runAnimation(float timepassed)
{
    Ogre::Vector3 movement = Ogre::Vector3::ZERO;

    timepassed *= mAnimSpeedMult;
    while(mCurrentAnim && mPlaying)
    {
        float targetTime = std::min(mStopTime, mCurrentTime+timepassed);
        if(mNextKey == mCurrentKeys->end() || mNextKey->first > targetTime)
        {
            movement += updatePosition(targetTime);
            mPlaying = (mLooping || mStopTime > targetTime);
            break;
        }

        float time = mNextKey->first;
        const std::string &evt = mNextKey->second;
        mNextKey++;

        movement += updatePosition(time);
        mPlaying = (mLooping || mStopTime > time);

        timepassed = targetTime - time;

        if(!handleEvent(time, evt))
            break;
    }

    return movement;
}

}
