#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DBREFGEOMETRYOBJECT_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DBREFGEOMETRYOBJECT_H

#include "objecttransform.hpp"
#include "recastmesh.hpp"

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <vector>

namespace DetourNavigator
{
    struct DbRefGeometryObject
    {
        std::int64_t mShapeId;
        ObjectTransform mObjectTransform;

        friend inline auto tie(const DbRefGeometryObject& v)
        {
            return std::tie(v.mShapeId, v.mObjectTransform);
        }

        friend inline bool operator<(const DbRefGeometryObject& l, const DbRefGeometryObject& r)
        {
            return tie(l) < tie(r);
        }
    };

    template <class ResolveMeshSource>
    inline std::vector<DbRefGeometryObject> makeDbRefGeometryObjects(const std::vector<MeshSource>& meshSources,
        ResolveMeshSource&& resolveMeshSource)
    {
        std::vector<DbRefGeometryObject> result;
        result.reserve(meshSources.size());
        std::transform(meshSources.begin(), meshSources.end(), std::back_inserter(result),
            [&] (const MeshSource& meshSource)
            {
                return DbRefGeometryObject {resolveMeshSource(meshSource), meshSource.mObjectTransform};
            });
        std::sort(result.begin(), result.end());
        return result;
    }
}

#endif
