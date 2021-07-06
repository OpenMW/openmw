#ifndef OPENMW_MWPHYSICS_RAYCASTING_H
#define OPENMW_MWPHYSICS_RAYCASTING_H

#include <osg/Vec3f>

#include "../mwworld/ptr.hpp"

#include "collisiontype.hpp"

namespace MWPhysics
{
    struct RayCastingResult
    {
        bool mHit;
        osg::Vec3f mHitPos;
        osg::Vec3f mHitNormal;
        MWWorld::Ptr mHitObject;
    };

    class RayCastingInterface
    {
        public:
            /// Get distance from \a point to the collision shape of \a target. Uses a raycast to find where the
            /// target vector hits the collision shape and then calculates distance from the intersection point.
            /// This can be used to find out how much nearer we need to move to the target for a "getHitContact" to be successful.
            /// \note Only Actor targets are supported at the moment.
            virtual float getHitDistance(const osg::Vec3f& point, const MWWorld::ConstPtr& target) const = 0;

            /// @param me Optional, a Ptr to ignore in the list of results. targets are actors to filter for, ignoring all other actors.
            virtual RayCastingResult castRay(const osg::Vec3f &from, const osg::Vec3f &to, const MWWorld::ConstPtr& ignore = MWWorld::ConstPtr(),
                    const std::vector<MWWorld::Ptr>& targets = std::vector<MWWorld::Ptr>(),
                    int mask = CollisionType_World|CollisionType_HeightMap|CollisionType_Actor|CollisionType_Door, int group=0xff) const = 0;

            virtual RayCastingResult castSphere(const osg::Vec3f& from, const osg::Vec3f& to, float radius) const = 0;

            /// Return true if actor1 can see actor2.
            virtual bool getLineOfSight(const MWWorld::ConstPtr& actor1, const MWWorld::ConstPtr& actor2) const = 0;
    };
}

#endif
