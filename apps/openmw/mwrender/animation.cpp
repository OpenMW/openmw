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
    , mLooping(false)
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

            mStartPosition = mNonAccumRoot->getPosition();
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


Ogre::Vector3 Animation::updatePosition(float time)
{
    mAnimState->setTimePosition(time);

    Ogre::Vector3 posdiff = Ogre::Vector3::ZERO;
    if(mNonAccumRoot)
    {
        /* Update the animation and get the non-accumulation root's difference from the
         * last update. */
        mEntityList.mSkelBase->getSkeleton()->setAnimationState(*mAnimState->getParent());
        posdiff = (mNonAccumRoot->getPosition() - mLastPosition) * mAccumulate;

        /* Translate the accumulation root back to compensate for the move. */
        mAccumRoot->translate(-posdiff);
        mLastPosition += posdiff;
    }
    return posdiff;
}

void Animation::resetPosition(float time)
{
    mAnimState->setTimePosition(time);

    mNextKey = mCurrentKeys->begin();
    while(mNextKey != mCurrentKeys->end() && mNextKey->first < time)
        mNextKey++;

    if(mNonAccumRoot)
    {
        mEntityList.mSkelBase->getSkeleton()->setAnimationState(*mAnimState->getParent());
        mLastPosition = mNonAccumRoot->getPosition();
        mAccumRoot->setPosition(mStartPosition - mLastPosition);
    }
}


float Animation::findStart(const std::string &groupname, const std::string &start)
{
    mNextKey = mCurrentKeys->end();
    if(mCurrentKeys->size() == 0)
        return 0.0f;

    if(groupname == "all")
    {
        mNextKey = mCurrentKeys->begin();
        return 0.0f;
    }

    std::string startmarker = groupname+": "+start;
    NifOgre::TextKeyMap::const_iterator iter;
    for(iter = mCurrentKeys->begin();iter != mCurrentKeys->end();iter++)
    {
        if(iter->second == startmarker)
            return iter->first;
    }
    return 0.0f;
}


void Animation::play(const std::string &groupname, const std::string &start, bool loop)
{
    try {
        if(mAnimState)
            mAnimState->setEnabled(false);
        mAnimState = mEntityList.mSkelBase->getAnimationState(groupname);
        mAnimState->setEnabled(true);
        mCurrentKeys = &mTextKeys[groupname];
        mLooping = loop;

        resetPosition(findStart(groupname, start));
    }
    catch(std::exception &e) {
        std::cerr<< e.what() <<std::endl;
    }
}

Ogre::Vector3 Animation::runAnimation(float timepassed)
{
    Ogre::Vector3 movement = Ogre::Vector3::ZERO;
    timepassed *= mAnimSpeedMult;
    while(mAnimState && timepassed > 0.0f)
    {
        float targetTime = mAnimState->getTimePosition() + timepassed;
        if(mNextKey == mCurrentKeys->end() || mNextKey->first > targetTime)
        {
            movement += updatePosition(targetTime);
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
            if(mLooping)
            {
                resetPosition(findStart(mAnimState->getAnimationName(), "loop start"));
                if(mAnimState->getTimePosition() >= time)
                    break;
            }
            continue;
        }
        if(evt == "stop")
        {
            if(mLooping)
            {
                resetPosition(findStart(mAnimState->getAnimationName(), "loop start"));
                if(mAnimState->getTimePosition() >= time)
                    break;
            }
            else if(mController)
                mController->markerEvent(time, evt);
            continue;
        }
        if(mController)
            mController->markerEvent(time, evt);
    }
    return movement;
}

}
