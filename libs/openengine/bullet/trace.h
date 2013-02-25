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
};

void newtrace(traceResults *results, const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::Vector3& BBHalfExtents, bool isInterior, OEngine::Physic::PhysicEngine* enginePass);

#endif
