#include "findoptimalpath.hpp"
#include "findoptimalpath/algorithm.hpp"
#include "findoptimalpath/any_angle_visitor.hpp"

namespace MWMechanics
{
    OptimalPath findOptimalPath(btCollisionWorld& collisionWorld, const btCollisionObject& actor,
            const btVector3& initial, const btVector3& goal, const FindOptimalPathConfig& config)
    {
        using FindOptimalPath::AnyAngleVisitor;
        using FindOptimalPath::Algorithm;
        using FindOptimalPath::HasNearCollisionFilter;
        using Visitor = AnyAngleVisitor<HasNearCollisionFilter>;
        Visitor visitor(collisionWorld, actor, initial, goal, config, HasNearCollisionFilter(config));
        return Algorithm<Visitor>(config, visitor).perform();
    }
}
