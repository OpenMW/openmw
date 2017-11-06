#ifndef OPENMW_MWMECHANICS_FINDOPTIMALPATH_DEBUG_H
#define OPENMW_MWMECHANICS_FINDOPTIMALPATH_DEBUG_H

#include "common.hpp"

#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>

#ifdef FIND_OPTIMAL_PATH_JSON
#include <extern/nlohmann/json.hpp>
#endif

#include <iomanip>
#include <iostream>

namespace MWMechanics
{
    namespace FindOptimalPath
    {
        using MWPhysics::Collision;

        struct DevNull
        {
            static DevNull& instance()
            {
                static DevNull value;
                return value;
            }
        };

        template <class T>
        DevNull& operator <<(DevNull& stream, const T&)
        {
            return stream;
        }

#ifdef FIND_OPTIMAL_PATH_DEBUG_OUTPUT
#define DEBUG_LOG std::cout
#else
#define DEBUG_LOG DevNull::instance()
#endif

#ifdef FIND_OPTIMAL_PATH_JSON
#define JSON_LOG std::cerr
#else
#define JSON_LOG DevNull::instance()
#endif

#ifdef FIND_OPTIMAL_PATH_DEBUG_OUTPUT
        inline std::ostream& operator <<(std::ostream& stream, const btVector3& value)
        {
            return stream
                << std::setprecision(std::numeric_limits<btScalar>::digits)
                << "(" << value.x() << ", " << value.y() << ", " << value.z() << ")";
        }

        inline std::ostream& operator <<(std::ostream& stream, const btMatrix3x3& value)
        {
            stream << "btMatrix3x3 {\n";
            for (int i = 0; i < 3; ++i) {
                const auto& row = value.getRow(i);
                stream << row.x() << ", " << row.y() << ", " << row.z() << ",\n";
            }
            return stream << "}";
        }

        inline std::ostream& operator <<(std::ostream& stream, const btTransform& value)
        {
            return stream
                << std::setprecision(std::numeric_limits<btScalar>::digits)
                << "btTransform {\n" << value.getBasis() << ",\n" << value.getOrigin() << "\n}";
        }
#endif

#if defined(FIND_OPTIMAL_PATH_DEBUG_OUTPUT) || defined(FIND_OPTIMAL_PATH_JSON)
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
                    return stream << "undefined(" << int(value) << ")";
            }
        }
#endif

#ifdef FIND_OPTIMAL_PATH_DEBUG_OUTPUT
        inline std::ostream& operator <<(std::ostream& stream, AabbSide value)
        {
            switch (value)
            {
                case AabbSide::X:
                    return stream << 'X';
                case AabbSide::Y:
                    return stream << 'Y';
                case AabbSide::Z:
                    return stream << 'Z';
                default:
                    return stream << int(value);
            }
        }
#endif

#if defined(FIND_OPTIMAL_PATH_DEBUG_OUTPUT) || defined(FIND_OPTIMAL_PATH_JSON)
        inline std::ostream& operator <<(std::ostream& stream, Transition::State value)
        {
            switch (value)
            {
                case Transition::State::Proposed:
                    return stream << "Proposed";
                case Transition::State::WithGroundedDestination:
                    return stream << "WithGroundedDestination";
                case Transition::State::WithoutCollisions:
                    return stream << "WithoutCollisions";
                case Transition::State::SubTraced:
                    return stream << "SubTraced";
                case Transition::State::Traced:
                    return stream << "Traced";
            }

            throw std::invalid_argument("Unhandled Transition::State: " + std::to_string(int(value)));
        }
#endif

        inline std::ostream& operator <<(std::ostream& stream, const PointInt& value)
        {
            return stream << "(" << value.x() << ", " << value.y() << ", " << value.z() << ")";
        }

#ifdef FIND_OPTIMAL_PATH_DEBUG_OUTPUT
        inline std::ostream& operator <<(std::ostream& stream, const Transition& value)
        {
            return stream
                << value.mId << "~" << value.mParentId
                << " " << value.mSource << " -> " << value.mDestination
                << " [" << value.mSourceInt << " -> " << value.mDestinationInt << "]"
                << " length: " << value.mSource.distance(value.mDestination)
                << " cost: " << value.mCost
                << " priority: " << value.mPriority
                << " depth: " << value.mDepth
                << " state: " << value.mState;
        }
#endif

#ifdef FIND_OPTIMAL_PATH_JSON
        inline nlohmann::json toJson(const btVector3& value)
        {
            nlohmann::json result;
            result["x"] = value.x();
            result["y"] = value.y();
            result["z"] = value.z();
            return result;
        }

        inline nlohmann::json withType(const btVector3& value, const std::string& type)
        {
            nlohmann::json result;
            result["type"] = type;
            result["position"] = toJson(value);
            return result;
        }

        inline nlohmann::json toJson(const PointInt& value)
        {
            nlohmann::json result;
            result["x"] = value.x();
            result["y"] = value.y();
            result["z"] = value.z();
            return result;
        }

        inline nlohmann::json toJson(Transition::State value)
        {
            std::ostringstream stream;
            stream << value;
            return stream.str();
        }

        inline nlohmann::json toJson(const Transition& value)
        {
            nlohmann::json result;
            result["id"] = value.mId;
            result["parent_id"] = value.mParentId;
            result["priority"] = value.mPriority;
            result["cost"] = value.mCost;
            result["depth"] = value.mDepth;
            result["source"] = toJson(value.mSource);
            result["destination"] = toJson(value.mDestination);
            result["source_int"] = toJson(value.mSourceInt);
            result["destination_int"] = toJson(value.mDestinationInt);
            result["state"] = toJson(value.mState);
            return result;
        }

        inline nlohmann::json withType(const Transition& value, const std::string& type)
        {
            nlohmann::json result;
            result["type"] = type;
            result["transition"] = toJson(value);
            return result;
        }

        inline nlohmann::json toJson(const Collision& value)
        {
            nlohmann::json result;
            result["point"] = toJson(value.mPoint);
            result["normal"] = toJson(value.mNormal);
            result["end"] = toJson(value.mEnd);
            std::ostringstream ptr;
            ptr << value.mObject;
            result["object"]["ptr"] = ptr.str();
            return result;
        }

        inline nlohmann::json withType(const Collision& value)
        {
            nlohmann::json result;
            result["collision"] = toJson(value);
            return result;
        }

        inline nlohmann::json toJson(const btScaledBvhTriangleMeshShape& shape, const btTransform& transform)
        {
            nlohmann::json result;
            nlohmann::json triangles;
            struct Callback : btTriangleCallback
            {
                nlohmann::json& triangles;
                const btTransform& transform;

                Callback(nlohmann::json& triangles, const btTransform& transform)
                    : triangles(triangles), transform(transform) {}

                void processTriangle(btVector3* triangle, int, int) override final
                {
                    triangles.push_back({toJson(transform(triangle[0])),
                                         toJson(transform(triangle[1])),
                                         toJson(transform(triangle[2]))});
                }
            } callback(triangles, transform);
            btVector3 aabbMin;
            btVector3 aabbMax;
            shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
            shape.processAllTriangles(&callback, aabbMin, aabbMax);
            result["triangles"] = triangles;
            return result;
        }

        inline nlohmann::json toJson(const btCollisionShape* shape, const btTransform& transform)
        {
            nlohmann::json result;
            btVector3 aabbMin;
            btVector3 aabbMax;
            shape->getAabb(transform, aabbMin, aabbMax);
            result["aabb"]["min"] = toJson(aabbMin);
            result["aabb"]["max"] = toJson(aabbMax);
            std::ostringstream type;
            type << BroadphaseNativeTypes(shape->getShapeType());
            result["type"] = type.str();
            if (shape->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
            {
                result["data"] = toJson(*static_cast<const btScaledBvhTriangleMeshShape*>(shape), transform);
            }
            return result;
        }

        inline nlohmann::json toJson(const btCollisionObject& object)
        {
            nlohmann::json result;
            std::ostringstream ptr;
            ptr << &object;
            result["ptr"] = ptr.str();
            result["origin"] = toJson(object.getWorldTransform().getOrigin());
            result["shape"] = toJson(object.getCollisionShape(), object.getWorldTransform());
            return result;
        }

        inline nlohmann::json withType(const btCollisionObject& value)
        {
            nlohmann::json result;
            result["object"] = toJson(value);
            return result;
        }
#else
        template <class ... T>
        int withType(T&& ...)
        {
            return 0;
        }

        template <class ... T>
        int toJson(T&& ...)
        {
            return 0;
        }
#endif
    }
}

#endif // OPENMW_MWMECHANICS_FINDOPTIMALPATH_DEBUG_H
