#ifndef OENGINE_BULLET_TRACE_H
#define OENGINE_BULLET_TRACE_H

#include <OgreVector3.h>


namespace OEngine
{
    namespace Physic
    {
        class PhysicEngine;
    }
}


struct traceResults
{
    Ogre::Vector3 endpos;
    Ogre::Vector3 planenormal;

    float fraction;

    int surfaceFlags;
    int contents;
    int entityNum;

    bool allsolid;
    bool startsolid;
};

void newtrace(traceResults* const results, const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::Vector3& BBExtents, const float rotation, bool isInterior, OEngine::Physic::PhysicEngine* enginePass);

#endif
