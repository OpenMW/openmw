#ifndef OPENMW_MWPHYSICS_MOVEMENTSOLVER_H
#define OPENMW_MWPHYSICS_MOVEMENTSOLVER_H

#include <map>

#include <osg/Vec3f>

class btCollisionWorld;

namespace MWWorld
{
    class Ptr;
}

namespace MWPhysics
{
    class Actor;
    struct ActorFrameData;
    struct WorldFrameData;

    class MovementSolver
    {
    private:
        ///Project a vector u on another vector v
        static inline osg::Vec3f project(const osg::Vec3f& u, const osg::Vec3f &v)
        {
            return v * (u * v);
            //            ^ dot product
        }

        ///Helper for computing the character sliding
        static inline osg::Vec3f slide(const osg::Vec3f& direction, const osg::Vec3f &planeNormal)
        {
            return direction - project(direction, planeNormal);
        }

    public:
        static osg::Vec3f traceDown(const MWWorld::Ptr &ptr, const osg::Vec3f& position, Actor* actor, btCollisionWorld* collisionWorld, float maxHeight);
        static void move(ActorFrameData& actor, float time, const btCollisionWorld* collisionWorld, WorldFrameData& worldData);
    };
}

#endif
