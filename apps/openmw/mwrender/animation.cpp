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
}

}
