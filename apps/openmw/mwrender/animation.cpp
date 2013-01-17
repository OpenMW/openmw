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
    , mStartPosition(0.0f)
    , mLastPosition(0.0f)
    , mTime(0.0f)
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
            Ogre::Bone *bone = boneiter.peekNext();
            const Ogre::Any &data = bone->getUserObjectBindings().getUserAny(NifOgre::sTextKeyExtraDataID);
            if(!data.isEmpty())
            {
                mTextKeys["all"] = Ogre::any_cast<NifOgre::TextKeyMap>(data);

                mAccumRoot = skelinst->getRootBone();
                mAccumRoot->setManuallyControlled(true);
                mNonAccumRoot = skelinst->getBone(bone->getHandle());

                mStartPosition = mNonAccumRoot->getPosition();
                mLastPosition = mStartPosition;
                break;
            }
            boneiter.moveNext();
        }

        if(boneiter.hasMoreElements())
        {
            asiter = aset->getAnimationStateIterator();
            while(asiter.hasMoreElements())
            {
                Ogre::AnimationState *state = asiter.getNext();
                Ogre::UserObjectBindings &bindings = boneiter.peekNext()->getUserObjectBindings();
                const Ogre::Any &data = bindings.getUserAny(std::string(NifOgre::sTextKeyExtraDataID)+"@"+state->getAnimationName());
                if(!data.isEmpty())
                    mTextKeys[state->getAnimationName()] = Ogre::any_cast<NifOgre::TextKeyMap>(data);
            }
        }
    }
}


std::vector<std::string> Animation::getAnimationNames()
{
    std::vector<std::string> anims;
    if(mEntityList.mSkelBase)
    {
        Ogre::AnimationStateSet *as = mEntityList.mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator ai = as->getAnimationStateIterator();
        while(ai.hasMoreElements())
            anims.push_back(ai.getNext()->getAnimationName());
    }
    return anims;
}


void Animation::setController(MWMechanics::CharacterController *controller)
{
    mController = controller;
}


void Animation::updatePosition(float time)
{
    mCurGroup.mAnimState->setTimePosition(time);
    mTime = time;

    if(mNonAccumRoot)
    {
        /* Update the animation and get the non-accumulation root's difference from the
         * last update. */
        mEntityList.mSkelBase->getSkeleton()->setAnimationState(*mCurGroup.mAnimState->getParent());
        Ogre::Vector3 posdiff = mNonAccumRoot->getPosition() - mLastPosition;

        /* Translate the accumulation root back to compensate for the move. */
        mAccumRoot->translate(-posdiff);
        mLastPosition += posdiff;

        if(mPtr.isInCell())
        {
            /* Finally, move the object based on how much the non-accumulation root moved. */
            Ogre::Vector3 newpos(mPtr.getRefData().getPosition().pos);
            newpos += mInsert->getOrientation() * posdiff;

            MWBase::World *world = MWBase::Environment::get().getWorld();
            world->moveObject(mPtr, newpos.x, newpos.y, newpos.z);
        }
    }
}

void Animation::resetPosition(float time)
{
    mCurGroup.mAnimState->setTimePosition(time);
    mTime = time;

    mCurGroup.mNext = mCurGroup.mStart;
    while(mCurGroup.mNext != mCurGroup.mTextKeys->end() && mCurGroup.mNext->first < time)
        mCurGroup.mNext++;

    if(mNonAccumRoot)
    {
        mEntityList.mSkelBase->getSkeleton()->setAnimationState(*mCurGroup.mAnimState->getParent());
        mLastPosition = mNonAccumRoot->getPosition();
        mAccumRoot->setPosition(mStartPosition - mLastPosition);
    }
}


bool Animation::findGroupTimes(const std::string &groupname, Animation::GroupTimes *times)
{
    times->mTextKeys = &mTextKeys[groupname];
    const NifOgre::TextKeyMap &textkeys = *times->mTextKeys;
    if(textkeys.size() == 0)
        return false;

    if(groupname == "all")
    {
        times->mStart = times->mLoopStart = textkeys.begin();
        times->mLoopStop = times->mStop = textkeys.end();
        times->mLoopStop--; times->mStop--;
        return true;
    }

    const std::string start = groupname+": start";
    const std::string startloop = groupname+": loop start";
    const std::string stop = groupname+": stop";
    const std::string stoploop = groupname+": loop stop";

    times->mStart = times->mLoopStart =
    times->mStop = times->mLoopStop = textkeys.end();

    NifOgre::TextKeyMap::const_iterator iter;
    for(iter = textkeys.begin();iter != textkeys.end();iter++)
    {
        if(start == iter->second)
        {
            times->mStart = iter;
            times->mLoopStart = iter;
        }
        else if(startloop == iter->second)
            times->mLoopStart = iter;
        else if(stoploop == iter->second)
            times->mLoopStop = iter;
        else if(stop == iter->second)
        {
            times->mStop = iter;
            if(times->mLoopStop == textkeys.end())
                times->mLoopStop = iter;
            return (times->mStart != textkeys.end());
        }
    }

    return false;
}


void Animation::playGroup(std::string groupname, int mode, int loops)
{
    GroupTimes times;

    try {
        if(!mEntityList.mSkelBase)
            throw std::runtime_error("Attempting to animate an inanimate object");

        std::transform(groupname.begin(), groupname.end(), groupname.begin(), ::tolower);
        times.mAnimState = mEntityList.mSkelBase->getAnimationState(groupname);
        times.mLoops = loops;

        if(!findGroupTimes(groupname, &times))
            throw std::runtime_error("Failed to find animation group "+groupname);
    }
    catch(std::exception &e) {
        std::cerr<< e.what() <<std::endl;
        return;
    }
    times.mNext = ((mode==2) ? times.mLoopStart : times.mStart);

    if(mode == 0 && mCurGroup.mAnimState)
        mNextGroup = times;
    else
    {
        if(mCurGroup.mAnimState)
            mCurGroup.mAnimState->setEnabled(false);
        mCurGroup = times;
        mNextGroup = GroupTimes();
        mCurGroup.mAnimState->setEnabled(true);
        resetPosition(mCurGroup.mNext->first);
    }
}

void Animation::runAnimation(float timepassed)
{
    while(mCurGroup.mAnimState && timepassed > 0.0f)
    {
        float targetTime = mTime + timepassed;
        if(mCurGroup.mNext != mCurGroup.mTextKeys->end() &&
           mCurGroup.mNext->first <= targetTime)
        {
            updatePosition(mCurGroup.mNext->first);
            timepassed = targetTime - mTime;

            if(mController)
                mController->markerEvent(mCurGroup.mNext->second);
            if(mCurGroup.mNext == mCurGroup.mLoopStop && mCurGroup.mLoops > 1)
            {
                mCurGroup.mLoops--;
                resetPosition(mCurGroup.mLoopStart->first);
                continue;
            }
            else if(mCurGroup.mNext == mCurGroup.mStop)
            {
                if(!mNextGroup.mAnimState)
                    break;

                mCurGroup.mAnimState->setEnabled(false);
                mCurGroup = mNextGroup;
                mNextGroup = GroupTimes();
                mCurGroup.mAnimState->setEnabled(true);
                resetPosition(mCurGroup.mStart->first);
                continue;
            }
            mCurGroup.mNext++;
            continue;
        }

        updatePosition(targetTime);
        timepassed = targetTime - mTime;
    }
}

}
