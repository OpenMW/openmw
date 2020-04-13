#ifndef OPENMW_MWPHYSICS_MOVEMENTSOLVER_H
#define OPENMW_MWPHYSICS_MOVEMENTSOLVER_H

#include <map>

#include <osg/Vec3f>

#include "../mwworld/ptr.hpp"

class btCollisionWorld;

namespace MWPhysics
{
    class Actor;

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
        static osg::Vec3f move(osg::Vec3f position, const MWWorld::Ptr &ptr, Actor* physicActor, const osg::Vec3f &movement, float time,
                               bool isFlying, float waterlevel, float slowFall, const btCollisionWorld* collisionWorld,
                               std::map<MWWorld::Ptr, MWWorld::Ptr>& standingCollisionTracker);
    };
}

#endif
