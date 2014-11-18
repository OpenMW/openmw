#ifndef OENGINE_BULLET_TRACE_H
#define OENGINE_BULLET_TRACE_H

#include <OgreVector3.h>


class btCollisionObject;


namespace OEngine
{
namespace Physic
{
    class PhysicEngine;
    class PhysicActor;

    struct ActorTracer
    {
        Ogre::Vector3 mEndPos;
        Ogre::Vector3 mPlaneNormal;
        const btCollisionObject* mHitObject;

        float mFraction;

        void doTrace(btCollisionObject *actor, const Ogre::Vector3 &start, const Ogre::Vector3 &end,
                     const PhysicEngine *enginePass);
        void findGround(const OEngine::Physic::PhysicActor* actor, const Ogre::Vector3 &start, const Ogre::Vector3 &end,
                        const PhysicEngine *enginePass);
    };
}
}

#endif
