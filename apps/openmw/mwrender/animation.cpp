#include "animation.hpp"

#include <OgreHardwarePixelBuffer.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>

namespace MWRender {

std::map<std::string, int> Animation::mUniqueIDs;

Animation::Animation(OEngine::Render::OgreRenderer& _rend)
    : insert(NULL)
    , mRend(_rend)
    , time(0.0f)
    , startTime(0.0f)
    , stopTime(0.0f)
    , animate(0)
    , rindexI()
    , tindexI()
    , shapeNumber(0)
    , shapeIndexI()
    , transformations(NULL)
    , textmappings(NULL)
    , base(NULL)
{
}

Animation::~Animation()
{
}

void Animation::startScript(std::string groupname, int mode, int loops)
{
    //If groupname is recognized set animate to true
    //Set the start time and stop time
    //How many times to loop
    if(groupname == "all")
    {
        animate = loops;
        time = startTime;
    }
    else if(textmappings)
    {
        std::string startName = groupname + ": loop start";
        std::string stopName = groupname + ": loop stop";

        bool first = false;

        if(loops > 1)
        {
            startName = groupname + ": loop start";
            stopName = groupname + ": loop stop";

            for(std::map<std::string, float>::iterator iter = textmappings->begin(); iter != textmappings->end(); iter++)
            {
                std::string current = iter->first.substr(0, startName.size());
                std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                std::string current2 = iter->first.substr(0, stopName.size());
                std::transform(current2.begin(), current2.end(), current2.begin(), ::tolower);

                if(current == startName)
                {
                    startTime = iter->second;
                        animate = loops;
                        time = startTime;
                        first = true;
                }
                if(current2 == stopName)
                {
                    stopTime = iter->second;
                    if(first)
                        break;
                }
            }
        }

        if(!first)
        {
            startName = groupname + ": start";
            stopName = groupname + ": stop";

            for(std::map<std::string, float>::iterator iter = textmappings->begin(); iter != textmappings->end(); iter++)
            {
                std::string current = iter->first.substr(0, startName.size());
                std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                std::string current2 = iter->first.substr(0, stopName.size());
                std::transform(current2.begin(), current2.end(), current2.begin(), ::tolower);

                if(current == startName)
                {
                    startTime = iter->second;
                        animate = loops;
                        time = startTime;
                        first = true;
                }
                if(current2 == stopName)
                {
                    stopTime = iter->second;
                    if(first)
                        break;
                }
            }
        }

    }

}


void Animation::stopScript()
{
    animate = 0;
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
    Ogre::SkeletonInstance* skel = base->getSkeleton();

    Ogre::Bone* b = skel->getRootBone();
    b->setOrientation(Ogre::Real(.3),Ogre::Real(.3),Ogre::Real(.3), Ogre::Real(.3));   //This is a trick

    skel->_updateTransforms();
    //skel->_notifyManualBonesDirty();

    base->getAllAnimationStates()->_notifyDirty();
    //base->_updateAnimation();
    //base->_notifyMoved();

    std::vector<Nif::NiKeyframeData>::iterator iter;
    int slot = 0;
    if(transformations)
    {
        for(iter = transformations->begin(); iter != transformations->end(); iter++)
        {
            if(time < iter->getStartTime() || time < startTime || time > iter->getStopTime())
            {
                slot++;
                continue;
            }

            float x;
            float x2;

            const std::vector<Ogre::Quaternion> &quats = iter->getQuat();
            const std::vector<float> &ttime = iter->gettTime();
            const std::vector<float> &rtime = iter->getrTime();
            const std::vector<Ogre::Vector3> &translist1 = iter->getTranslist1();

            int rindexJ = rindexI[slot];
            timeIndex(time, rtime, rindexI[slot], rindexJ, x2);

            int tindexJ = tindexI[slot];
            timeIndex(time, ttime, tindexI[slot], tindexJ, x);

            Ogre::Vector3 t;
            Ogre::Quaternion r;

            bool bTrans = translist1.size() > 0;
            bool bQuats = quats.size() > 0;
            if(skel->hasBone(iter->getBonename()))
            {
                Ogre::Bone* bone = skel->getBone(iter->getBonename());
                if(bTrans)
                {
                    Ogre::Vector3 v1 = translist1[tindexI[slot]];
                    Ogre::Vector3 v2 = translist1[tindexJ];
                    t = (v1 + (v2 - v1) * x);
                    bone->setPosition(t);
                }
                if(bQuats)
                {
                    r = Ogre::Quaternion::Slerp(x2, quats[rindexI[slot]], quats[rindexJ], true);
                    bone->setOrientation(r);
                }
            }

            slot++;
        }
        skel->_updateTransforms();
        base->getAllAnimationStates()->_notifyDirty();
    }
}

}
