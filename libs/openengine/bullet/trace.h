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


class btCollisionObject;


struct traceResults
{
    Ogre::Vector3 endpos;
    Ogre::Vector3 planenormal;

    float fraction;
};

void actortrace(traceResults *results, btCollisionObject *actor, const Ogre::Vector3& start, const Ogre::Vector3& end, OEngine::Physic::PhysicEngine *enginePass);

#endif
