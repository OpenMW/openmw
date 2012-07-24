#include "animation.hpp"

#include <OgreHardwarePixelBuffer.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>


namespace MWRender
{

Animation::Animation(OEngine::Render::OgreRenderer& _rend)
    : mInsert(NULL)
    , mRend(_rend)
    , mTime(0.0f)
    , mAnimate(0)
    , mSkipFrame(false)
{
}

Animation::~Animation()
{
    Ogre::SceneManager *sceneMgr = mInsert->getCreator();
    for(size_t i = 0;i < mEntityList.mEntities.size();i++)
        sceneMgr->destroyEntity(mEntityList.mEntities[i]);
    mEntityList.mEntities.clear();
}


struct checklow {
    bool operator()(const char &a, const char &b) const
    {
        return ::tolower(a) == ::tolower(b);
    }
};

bool Animation::findGroupTimes(const std::string &groupname, float *starttime, float *stoptime, float *loopstarttime, float *loopstoptime)
{
    const std::string &start = groupname+": start";
    const std::string &startloop = groupname+": loop start";
    const std::string &stop = groupname+": stop";
    const std::string &stoploop = groupname+": loop stop";

    *starttime = -1.0f;
    *stoptime  = -1.0f;
    *loopstarttime = -1.0f;
    *loopstoptime  = -1.0f;

    NifOgre::TextKeyMap::const_iterator iter;
    for(iter = mTextKeys.begin();iter != mTextKeys.end();iter++)
    {
        if(*starttime >= 0.0f && *stoptime >= 0.0f && *loopstarttime >= 0.0f && *loopstoptime >= 0.0f)
            return true;

        std::string::const_iterator strpos = iter->second.begin();
        std::string::const_iterator strend = iter->second.end();

        while(strpos != strend)
        {
            size_t strlen = strend-strpos;
            std::string::const_iterator striter;

            if(start.size() <= strlen &&
               ((striter=std::mismatch(strpos, strend, start.begin(), checklow()).first) == strend ||
                *striter == '\r' || *striter == '\n'))
            {
                *starttime = iter->first;
                *loopstarttime = iter->first;
            }
            else if(startloop.size() <= strlen &&
                    ((striter=std::mismatch(strpos, strend, startloop.begin(), checklow()).first) == strend ||
                     *striter == '\r' || *striter == '\n'))
            {
                *loopstarttime = iter->first;
            }
            else if(stoploop.size() <= strlen &&
                    ((striter=std::mismatch(strpos, strend, stoploop.begin(), checklow()).first) == strend ||
                     *striter == '\r' || *striter == '\n'))
            {
                *loopstoptime = iter->first;
            }
            else if(stop.size() <= strlen &&
                    ((striter=std::mismatch(strpos, strend, stop.begin(), checklow()).first) == strend ||
                     *striter == '\r' || *striter == '\n'))
            {
                *stoptime = iter->first;
                if(*loopstoptime < 0.0f)
                    *loopstoptime = iter->first;
                break;
            }

            strpos = std::find(strpos+1, strend, '\n');
            while(strpos != strend && *strpos == '\n')
                strpos++;
        }
    }

    return (*starttime >= 0.0f && *stoptime >= 0.0f && *loopstarttime >= 0.0f && *loopstoptime >= 0.0f);
}


void Animation::playGroup(std::string groupname, int mode, int loops)
{
    float start, stop, loopstart, loopstop;

    if(groupname == "all")
    {
        start = loopstart = 0.0f;
        loopstop = stop = 0.0f;

        if(mEntityList.mSkelBase)
        {
            Ogre::AnimationStateSet *aset = mEntityList.mSkelBase->getAllAnimationStates();
            Ogre::AnimationStateIterator as = aset->getAnimationStateIterator();
            while(as.hasMoreElements())
            {
                Ogre::AnimationState *state = as.getNext();
                loopstop = stop = state->getLength();
                break;
            }
        }
    }
    else if(!findGroupTimes(groupname, &start, &stop, &loopstart, &loopstop))
        throw std::runtime_error("Failed to find animation group "+groupname);

    // FIXME: mode = 0 not yet supported
    if(mode == 0)
        mode = 1;

    mStartTime = start;
    mStopTime = stop;
    mLoopStartTime = loopstart;
    mLoopStopTime = loopstop;

    mAnimate = loops;
    mTime = ((mode==1) ? mStartTime : mLoopStartTime);
}

void Animation::skipAnim()
{
    mSkipFrame = true;
}

void Animation::runAnimation(float timepassed)
{
    if(mAnimate > 0 && !mSkipFrame)
    {
        mTime += timepassed;
        if(mTime >= mLoopStopTime)
        {
            if(mAnimate > 1)
            {
                mAnimate--;
                mTime = mTime - mLoopStopTime + mLoopStartTime;
            }
            else if(mTime >= mStopTime)
            {
                mAnimate--;
                mTime = mStopTime;
            }
        }

        if(mEntityList.mSkelBase)
        {
            Ogre::AnimationStateSet *aset = mEntityList.mSkelBase->getAllAnimationStates();
            Ogre::AnimationStateIterator as = aset->getAnimationStateIterator();
            while(as.hasMoreElements())
            {
                Ogre::AnimationState *state = as.getNext();
                state->setTimePosition(mTime);
            }
        }
    }
    mSkipFrame = false;
}

}
