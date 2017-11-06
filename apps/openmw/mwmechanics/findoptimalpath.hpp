#ifndef OPENMW_MECHANICS_COLLISIONPATHFINDER_H
#define OPENMW_MECHANICS_COLLISIONPATHFINDER_H

#include <LinearMath/btVector3.h>

#include <vector>
#include <limits>

class btCollisionWorld;
class btCollisionObject;

namespace MWMechanics
{
    struct FindOptimalPathConfig
    {
        bool mAllowFly = false;
        btVector3 mActorHalfExtents {1, 1, 1};
        std::size_t mMaxIterations = std::numeric_limits<std::size_t>::max();
        std::size_t mMaxDepth = std::numeric_limits<std::size_t>::max();
        btScalar mHasNearCollisionFilterFactor = 0.33f;
        btScalar mHasNearCollisionFilterReduceFactor = 0.5;
        btScalar mHorizontalMarginFactor = 2;
        btScalar mSpaceScailingFactor = 16;
    };

    struct OptimalPath
    {
        bool mReachMaxIterations;
        std::size_t mIterations;
        std::vector<btVector3> mPoints;
    };

    OptimalPath findOptimalPath(btCollisionWorld& collisionWorld, const btCollisionObject& actor,
            const btVector3& initial, const btVector3& goal, const FindOptimalPathConfig& config);

} // MWMechanics

#endif // OPENMW_MECHANICS_COLLISIONPATHFINDER_H
