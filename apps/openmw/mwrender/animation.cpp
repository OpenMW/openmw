#include "animation.hpp"

#include <OgreSkeletonManager.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWRender
{

Animation::Animation(const MWWorld::Ptr &ptr)
    : mPtr(ptr)
    , mInsert(NULL)
    , mAccumRoot(NULL)
    , mNonAccumRoot(NULL)
    , mStartPosition(0.0f)
    , mLastPosition(0.0f)
    , mTime(0.0f)
    , mCurGroup(mTextKeys.end())
    , mNextGroup(mTextKeys.end())
    , mSkipFrame(false)
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

    mEntityList = NifOgre::NIFLoader::createEntities(mInsert, model);
    if(mEntityList.mSkelBase)
    {
        Ogre::SkeletonInstance *skelinst = mEntityList.mSkelBase->getSkeleton();
        // Would be nice if Ogre::SkeletonInstance allowed access to the 'master' Ogre::SkeletonPtr.
        Ogre::SkeletonManager &skelMgr = Ogre::SkeletonManager::getSingleton();
        Ogre::SkeletonPtr skel = skelMgr.getByName(skelinst->getName());
        Ogre::Skeleton::BoneIterator iter = skel->getBoneIterator();
        while(iter.hasMoreElements())
        {
            Ogre::Bone *bone = iter.getNext();
            const Ogre::Any &data = bone->getUserObjectBindings().getUserAny(NifOgre::sTextKeyExtraDataID);
            if(!data.isEmpty())
            {
                mTextKeys = Ogre::any_cast<NifOgre::TextKeyMap>(data);
                mNextGroup = mCurGroup = GroupTimes(mTextKeys.begin());

                mAccumRoot = skelinst->getRootBone();
                mAccumRoot->setManuallyControlled(true);
                mNonAccumRoot = skelinst->getBone(bone->getHandle());

                mStartPosition = mNonAccumRoot->getPosition();
                mLastPosition = mStartPosition;
                break;
            }
        }

        if(mTextKeys.size() > 0)
        {
            Ogre::AnimationStateSet *aset = mEntityList.mSkelBase->getAllAnimationStates();
            Ogre::AnimationStateIterator as = aset->getAnimationStateIterator();
            while(as.hasMoreElements())
            {
                Ogre::AnimationState *state = as.getNext();
                state->setEnabled(false);
                state->setLoop(false);
            }
        }
    }
}


void Animation::updatePosition(float time)
{
    mCurGroup.mAnimState->setTimePosition(time);
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
    if(mNonAccumRoot)
    {
        mEntityList.mSkelBase->getSkeleton()->setAnimationState(*mCurGroup.mAnimState->getParent());
        mLastPosition = mNonAccumRoot->getPosition();
        mAccumRoot->setPosition(mStartPosition - mLastPosition);
    }
}


bool Animation::findGroupTimes(const std::string &groupname, Animation::GroupTimes *times)
{
    const std::string start = groupname+": start";
    const std::string startloop = groupname+": loop start";
    const std::string stop = groupname+": stop";
    const std::string stoploop = groupname+": loop stop";

    NifOgre::TextKeyMap::const_iterator iter;
    for(iter = mTextKeys.begin();iter != mTextKeys.end();iter++)
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
            if(times->mLoopStop == mTextKeys.end())
                times->mLoopStop = iter;
            return (times->mStart != mTextKeys.end());
        }
    }

    return false;
}


void Animation::playGroup(std::string groupname, int mode, int loops)
{
    GroupTimes times(mTextKeys.end());

    try {
        if(mTextKeys.size() == 0)
            throw std::runtime_error("Trying to animate an unanimate object");

        std::transform(groupname.begin(), groupname.end(), groupname.begin(), ::tolower);
        times.mAnimState = mEntityList.mSkelBase->getAnimationState(groupname);
        times.mLoops = loops;

        if(groupname == "all")
        {
            times.mStart = times.mLoopStart = mTextKeys.begin();
            times.mLoopStop = times.mStop = mTextKeys.end();
            times.mLoopStop--; times.mStop--;
        }
        else if(!findGroupTimes(groupname, &times))
            throw std::runtime_error("Failed to find animation group "+groupname);
    }
    catch(std::exception &e) {
        std::cerr<< e.what() <<std::endl;
        return;
    }

    if(mode == 0 && mCurGroup.mLoops > 0)
        mNextGroup = times;
    else
    {
        if(mCurGroup.mAnimState)
            mCurGroup.mAnimState->setEnabled(false);
        mCurGroup = times;
        mNextGroup = GroupTimes(mTextKeys.end());
        mTime = ((mode==2) ? mCurGroup.mLoopStart : mCurGroup.mStart)->first;
        mCurGroup.mAnimState->setEnabled(true);
        resetPosition(mTime);
    }
}

void Animation::skipAnim()
{
    mSkipFrame = true;
}

void Animation::runAnimation(float timepassed)
{
    if(mCurGroup.mAnimState && !mSkipFrame)
    {
        mTime += timepassed;
    recheck:
        if(mTime >= mCurGroup.mLoopStop->first)
        {
            if(mCurGroup.mLoops > 1)
            {
                mCurGroup.mLoops--;
                updatePosition(mCurGroup.mLoopStop->first);
                mTime = mTime - mCurGroup.mLoopStop->first + mCurGroup.mLoopStart->first;
                resetPosition(mCurGroup.mLoopStart->first);
                goto recheck;
            }
            else if(mTime >= mCurGroup.mStop->first)
            {
                if(mNextGroup.mLoops > 0)
                {
                    updatePosition(mCurGroup.mStop->first);
                    mTime = mTime - mCurGroup.mStop->first + mNextGroup.mStart->first;
                    mCurGroup.mAnimState->setEnabled(false);
                    mCurGroup = mNextGroup;
                    mNextGroup = GroupTimes(mTextKeys.end());
                    mCurGroup.mAnimState->setEnabled(true);
                    resetPosition(mNextGroup.mStart->first);
                    goto recheck;
                }
                mTime = mCurGroup.mStop->first;
            }
        }

        updatePosition(mTime);
    }
    mSkipFrame = false;
}

}
