#include "animation.hpp"

#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>

#include "../mwbase/environment.hpp"
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
    , mStartPosition(0.0f)
    , mLastPosition(0.0f)
    , mCurrentKeys(NULL)
    , mAnimState(NULL)
    , mPlaying(false)
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


void Animation::createEntityList(Ogre::SceneNode *node, const std::string &model)
{
    mInsert = node;
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

        Ogre::SkeletonInstance *skelinst = mEntityList.mSkelBase->getSkeleton();
        // Would be nice if Ogre::SkeletonInstance allowed access to the 'master' Ogre::SkeletonPtr.
        Ogre::SkeletonManager &skelMgr = Ogre::SkeletonManager::getSingleton();
        Ogre::SkeletonPtr skel = skelMgr.getByName(skelinst->getName());
        Ogre::Skeleton::BoneIterator boneiter = skel->getBoneIterator();
        while(boneiter.hasMoreElements())
        {
            Ogre::Bone *bone = boneiter.getNext();
            Ogre::UserObjectBindings &bindings = bone->getUserObjectBindings();
            const Ogre::Any &data = bindings.getUserAny(NifOgre::sTextKeyExtraDataID);
            if(data.isEmpty() || !Ogre::any_cast<bool>(data))
                continue;

            mAccumRoot = skelinst->getRootBone();
            mAccumRoot->setManuallyControlled(true);
            mNonAccumRoot = skelinst->getBone(bone->getHandle());

            mStartPosition = mNonAccumRoot->getInitialPosition();
            mLastPosition = mStartPosition;

            asiter = aset->getAnimationStateIterator();
            while(asiter.hasMoreElements())
            {
                Ogre::AnimationState *state = asiter.getNext();
                const Ogre::Any &groupdata = bindings.getUserAny(std::string(NifOgre::sTextKeyExtraDataID)+
                                                                 "@"+state->getAnimationName());
                if(!groupdata.isEmpty())
                    mTextKeys[state->getAnimationName()] = Ogre::any_cast<NifOgre::TextKeyMap>(groupdata);
            }

            break;
        }

        // Set the bones as manually controlled since we're applying the
        // transformations manually (needed if we want to apply an animation
        // from one skeleton onto another).
        boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);
    }
}


bool Animation::hasAnimation(const std::string &anim)
{
    return mEntityList.mSkelBase && mEntityList.mSkelBase->hasAnimationState(anim);
}


void Animation::setController(MWMechanics::CharacterController *controller)
{
    mController = controller;
}


void Animation::setAccumulation(const Ogre::Vector3 &accum)
{
    mAccumulate = accum;
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


Ogre::Vector3 Animation::updatePosition(float time)
{
    Ogre::SkeletonInstance *skel = mEntityList.mSkelBase->getSkeleton();
    mAnimState->setTimePosition(time);
    applyAnimation(skel->getAnimation(mAnimState->getAnimationName()), mAnimState->getTimePosition(), skel);

    Ogre::Vector3 posdiff = Ogre::Vector3::ZERO;
    if(mNonAccumRoot)
    {
        /* Get the non-accumulation root's difference from the last update. */
        posdiff = (mNonAccumRoot->getPosition() - mLastPosition) * mAccumulate;

        /* Translate the accumulation root back to compensate for the move. */
        mAccumRoot->translate(-posdiff);
        mLastPosition += posdiff;
    }
    return posdiff;
}

void Animation::reset(const std::string &marker)
{
    mNextKey = mCurrentKeys->begin();
    while(mNextKey != mCurrentKeys->end() && mNextKey->second != marker)
        mNextKey++;

    Ogre::SkeletonInstance *skel = mEntityList.mSkelBase->getSkeleton();
    if(mNextKey != mCurrentKeys->end())
        mAnimState->setTimePosition(mNextKey->first);
    else
    {
        mNextKey = mCurrentKeys->begin();
        mAnimState->setTimePosition(0.0f);
    }
    applyAnimation(skel->getAnimation(mAnimState->getAnimationName()), mAnimState->getTimePosition(), skel);

    if(mNonAccumRoot)
    {
        mLastPosition = mNonAccumRoot->getPosition();
        mAccumRoot->setPosition(mStartPosition - mLastPosition);
    }
}


void Animation::play(const std::string &groupname, const std::string &start, bool loop)
{
    try {
        mAnimState = mEntityList.mSkelBase->getAnimationState(groupname);
        mAnimState->setLoop(loop);

        mCurrentKeys = &mTextKeys[groupname];
        reset(start);
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
    while(mAnimState && mPlaying && timepassed > 0.0f)
    {
        float targetTime = mAnimState->getTimePosition() + timepassed;
        if(mNextKey == mCurrentKeys->end() || mNextKey->first > targetTime)
        {
            movement += updatePosition(targetTime);
            mPlaying = (mAnimState->getLoop() || mAnimState->getLength() >= targetTime);
            break;
        }

        float time = mNextKey->first;
        const std::string &evt = mNextKey->second;
        mNextKey++;

        movement += updatePosition(time);
        timepassed = targetTime - time;

        if(evt == "start" || evt == "loop start")
        {
            /* Do nothing */
            continue;
        }
        if(evt == "loop stop")
        {
            if(mAnimState->getLoop())
            {
                reset("loop start");
                if(mAnimState->getTimePosition() >= time)
                    break;
            }
            continue;
        }
        if(evt == "stop")
        {
            if(mAnimState->getLoop())
            {
                reset("loop start");
                if(mAnimState->getTimePosition() >= time)
                    break;
            }
            else
            {
                mPlaying = false;
                if(mController)
                    mController->markerEvent(time, evt);
            }
            continue;
        }
        if(mController)
            mController->markerEvent(time, evt);
    }

    return movement;
}

}
