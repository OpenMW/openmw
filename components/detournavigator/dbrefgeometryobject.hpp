#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DBREFGEOMETRYOBJECT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DBREFGEOMETRYOBJECT_H

#include "objecttransform.hpp"
#include "recastmesh.hpp"

#include <components/misc/typetraits.hpp>

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace DetourNavigator
{
    struct DbRefGeometryObject
    {
        std::int64_t mShapeId;
        ObjectTransform mObjectTransform;

        friend inline auto tie(const DbRefGeometryObject& v) { return std::tie(v.mShapeId, v.mObjectTransform); }

        friend inline bool operator<(const DbRefGeometryObject& l, const DbRefGeometryObject& r)
        {
            return tie(l) < tie(r);
        }
    };

    template <class ResolveMeshSource>
    inline auto makeDbRefGeometryObjects(
        const std::vector<MeshSource>& meshSources, ResolveMeshSource&& resolveMeshSource)
        -> std::conditional_t<Misc::isOptional<std::decay_t<decltype(resolveMeshSource(meshSources.front()))>>,
            std::variant<std::vector<DbRefGeometryObject>, MeshSource>, std::vector<DbRefGeometryObject>>
    {
        std::vector<DbRefGeometryObject> result;
        result.reserve(meshSources.size());
        for (const MeshSource& meshSource : meshSources)
        {
            const auto shapeId = resolveMeshSource(meshSource);
            if constexpr (Misc::isOptional<std::decay_t<decltype(shapeId)>>)
            {
                if (!shapeId.has_value())
                    return meshSource;
                result.push_back(DbRefGeometryObject{ *shapeId, meshSource.mObjectTransform });
            }
            else
                result.push_back(DbRefGeometryObject{ shapeId, meshSource.mObjectTransform });
        }
        std::sort(result.begin(), result.end());
        return result;
    }
}

#endif
