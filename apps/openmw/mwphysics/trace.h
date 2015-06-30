#ifndef OENGINE_BULLET_TRACE_H
#define OENGINE_BULLET_TRACE_H

#include <osg/Vec3f>

class btCollisionObject;
class btCollisionWorld;


namespace MWPhysics
{
    class Actor;

    struct ActorTracer
    {
        osg::Vec3f mEndPos;
        osg::Vec3f mPlaneNormal;
        const btCollisionObject* mHitObject;

        float mFraction;

        void doTrace(btCollisionObject *actor, const osg::Vec3f& start, const osg::Vec3f& end, btCollisionWorld* world);
        void findGround(const Actor* actor, const osg::Vec3f& start, const osg::Vec3f& end, btCollisionWorld* world);
    };
}

#endif
