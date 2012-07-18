#include "animation.hpp"

#include <OgreHardwarePixelBuffer.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>

namespace MWRender
{
std::map<std::string, int> Animation::sUniqueIDs;

Animation::Animation(OEngine::Render::OgreRenderer& _rend)
    : mInsert(NULL)
    , mRend(_rend)
    , mTime(0.0f)
    , mStartTime(0.0f)
    , mStopTime(0.0f)
    , mAnimate(0)
    , mRindexI()
    , mTindexI()
    , mShapeNumber(0)
    , mShapeIndexI()
    , mTransformations(NULL)
    , mTextmappings(NULL)
{
}

Animation::~Animation()
{
    Ogre::SceneManager *sceneMgr = mInsert->getCreator();
    for(size_t i = 0;i < mEntityList.mEntities.size();i++)
        sceneMgr->destroyEntity(mEntityList.mEntities[i]);
    mEntityList.mEntities.clear();
}

void Animation::startScript(std::string groupname, int mode, int loops)
{
    //If groupname is recognized set animate to true
    //Set the start time and stop time
    //How many times to loop
    if(groupname == "all")
    {
        mAnimate = loops;
        mTime = mStartTime;
    }
    else if(mTextmappings)
    {
        std::string startName = groupname + ": loop start";
        std::string stopName = groupname + ": loop stop";

        bool first = false;

        if(loops > 1)
        {
            startName = groupname + ": loop start";
            stopName = groupname + ": loop stop";

            for(std::map<std::string, float>::iterator iter = mTextmappings->begin(); iter != mTextmappings->end(); iter++)
            {
                std::string current = iter->first.substr(0, startName.size());
                std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                std::string current2 = iter->first.substr(0, stopName.size());
                std::transform(current2.begin(), current2.end(), current2.begin(), ::tolower);

                if(current == startName)
                {
                    mStartTime = iter->second;
                        mAnimate = loops;
                        mTime = mStartTime;
                        first = true;
                }
                if(current2 == stopName)
                {
                    mStopTime = iter->second;
                    if(first)
                        break;
                }
            }
        }

        if(!first)
        {
            startName = groupname + ": start";
            stopName = groupname + ": stop";

            for(std::map<std::string, float>::iterator iter = mTextmappings->begin(); iter != mTextmappings->end(); iter++)
            {
                std::string current = iter->first.substr(0, startName.size());
                std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                std::string current2 = iter->first.substr(0, stopName.size());
                std::transform(current2.begin(), current2.end(), current2.begin(), ::tolower);

                if(current == startName)
                {
                    mStartTime = iter->second;
                        mAnimate = loops;
                        mTime = mStartTime;
                        first = true;
                }
                if(current2 == stopName)
                {
                    mStopTime = iter->second;
                    if(first)
                        break;
                }
            }
        }

    }

}


void Animation::stopScript()
{
    mAnimate = 0;
}


bool Animation::timeIndex(float time, const std::vector<float> &times, int &i, int &j, float &x)
{
    size_t count;
    if((count=times.size()) == 0)
        return false;

    if(time <= times[0])
    {
        i = j = 0;
        x = 0.0;
        return true;
    }
    if(time >= times[count-1])
    {
        i = j = count - 1;
        x = 0.0;
        return true;
    }

    if(i < 0 || (size_t)i >= count)
        i = 0;

    float tI = times[i];
    if(time > tI)
    {
        j = i + 1;
        float tJ;
        while(time >= (tJ=times[j]))
        {
            i = j++;
            tI = tJ;
        }
        x = (time-tI) / (tJ-tI);
        return true;
    }

    if(time < tI)
    {
        j = i - 1;
        float tJ;
        while(time <= (tJ=times[j]))
        {
            i = j--;
            tI = tJ;
        }
        x = (time-tI) / (tJ-tI);
        return true;
    }

    j = i;
    x = 0.0;
    return true;
}

void Animation::handleAnimationTransforms()
{
}

}
