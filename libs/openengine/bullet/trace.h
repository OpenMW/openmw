#ifndef OENGINE_BULLET_TRACE_H
#define OENGINE_BULLET_TRACE_H

#include <OgreVector3.h>


class btCollisionObject;


namespace OEngine
{
namespace Physic
{
    class PhysicEngine;

    struct ActorTracer
    {
        Ogre::Vector3 mEndPos;
        Ogre::Vector3 mPlaneNormal;

        float mFraction;

        void doTrace(btCollisionObject *actor, const Ogre::Vector3 &start, const Ogre::Vector3 &end,
                     const PhysicEngine *enginePass);
        void findGround(btCollisionObject *actor, const Ogre::Vector3 &start, const Ogre::Vector3 &end,
                        const PhysicEngine *enginePass);
    };
}
}

#endif
