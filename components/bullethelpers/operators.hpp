#ifndef OPENMW_COMPONENTS_BULLETHELPERS_OPERATORS_H
#define OPENMW_COMPONENTS_BULLETHELPERS_OPERATORS_H

#include <iomanip>
#include <limits>
#include <ostream>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <LinearMath/btTransform.h>

inline std::ostream& operator <<(std::ostream& stream, const btVector3& value)
{
    return stream << "btVector3(" << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.x()
                  << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.y()
                  << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.z()
                  << ')';
}

inline std::ostream& operator <<(std::ostream& stream, BroadphaseNativeTypes value)
{
    switch (value)
    {
#ifndef SHAPE_NAME
#define SHAPE_NAME(name) case name: return stream << #name;
        SHAPE_NAME(BOX_SHAPE_PROXYTYPE)
        SHAPE_NAME(TRIANGLE_SHAPE_PROXYTYPE)
        SHAPE_NAME(TETRAHEDRAL_SHAPE_PROXYTYPE)
        SHAPE_NAME(CONVEX_TRIANGLEMESH_SHAPE_PROXYTYPE)
        SHAPE_NAME(CONVEX_HULL_SHAPE_PROXYTYPE)
        SHAPE_NAME(CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE)
        SHAPE_NAME(CUSTOM_POLYHEDRAL_SHAPE_TYPE)
        SHAPE_NAME(IMPLICIT_CONVEX_SHAPES_START_HERE)
        SHAPE_NAME(SPHERE_SHAPE_PROXYTYPE)
        SHAPE_NAME(MULTI_SPHERE_SHAPE_PROXYTYPE)
        SHAPE_NAME(CAPSULE_SHAPE_PROXYTYPE)
        SHAPE_NAME(CONE_SHAPE_PROXYTYPE)
        SHAPE_NAME(CONVEX_SHAPE_PROXYTYPE)
        SHAPE_NAME(CYLINDER_SHAPE_PROXYTYPE)
        SHAPE_NAME(UNIFORM_SCALING_SHAPE_PROXYTYPE)
        SHAPE_NAME(MINKOWSKI_SUM_SHAPE_PROXYTYPE)
        SHAPE_NAME(MINKOWSKI_DIFFERENCE_SHAPE_PROXYTYPE)
        SHAPE_NAME(BOX_2D_SHAPE_PROXYTYPE)
        SHAPE_NAME(CONVEX_2D_SHAPE_PROXYTYPE)
        SHAPE_NAME(CUSTOM_CONVEX_SHAPE_TYPE)
        SHAPE_NAME(CONCAVE_SHAPES_START_HERE)
        SHAPE_NAME(TRIANGLE_MESH_SHAPE_PROXYTYPE)
        SHAPE_NAME(SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
        SHAPE_NAME(FAST_CONCAVE_MESH_PROXYTYPE)
        SHAPE_NAME(TERRAIN_SHAPE_PROXYTYPE)
        SHAPE_NAME(GIMPACT_SHAPE_PROXYTYPE)
        SHAPE_NAME(MULTIMATERIAL_TRIANGLE_MESH_PROXYTYPE)
        SHAPE_NAME(EMPTY_SHAPE_PROXYTYPE)
        SHAPE_NAME(STATIC_PLANE_PROXYTYPE)
        SHAPE_NAME(CUSTOM_CONCAVE_SHAPE_TYPE)
        SHAPE_NAME(CONCAVE_SHAPES_END_HERE)
        SHAPE_NAME(COMPOUND_SHAPE_PROXYTYPE)
        SHAPE_NAME(SOFTBODY_SHAPE_PROXYTYPE)
        SHAPE_NAME(HFFLUID_SHAPE_PROXYTYPE)
        SHAPE_NAME(HFFLUID_BUOYANT_CONVEX_SHAPE_PROXYTYPE)
        SHAPE_NAME(INVALID_SHAPE_PROXYTYPE)
        SHAPE_NAME(MAX_BROADPHASE_COLLISION_TYPES)
#undef SHAPE_NAME
#endif
        default:
            return stream << "undefined(" << static_cast<int>(value) << ")";
    }
}

#endif
