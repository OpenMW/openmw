#include "navmeshdbutils.hpp"
#include "navmeshdb.hpp"
#include "recastmesh.hpp"

#include <components/debug/debuglog.hpp>

#include <cassert>

namespace DetourNavigator
{
    namespace
    {
        ShapeId getShapeId(NavMeshDb& db, const std::string& name, ShapeType type, const std::string& hash, ShapeId& nextShapeId)
        {
            const Sqlite3::ConstBlob hashData {hash.data(), static_cast<int>(hash.size())};
            if (const auto existingShapeId = db.findShapeId(name, type, hashData))
                return *existingShapeId;
            const ShapeId newShapeId = nextShapeId;
            db.insertShape(newShapeId, name, type, hashData);
            Log(Debug::Verbose) << "Added " << name << " " << type << " shape to navmeshdb with id " << newShapeId;
            ++nextShapeId.t;
            return newShapeId;
        }
    }

    ShapeId resolveMeshSource(NavMeshDb& db, const MeshSource& source, ShapeId& nextShapeId)
    {
        switch (source.mAreaType)
        {
            case AreaType_null:
                return getShapeId(db, source.mShape->mFileName, ShapeType::Avoid, source.mShape->mFileHash, nextShapeId);
            case AreaType_ground:
                return getShapeId(db, source.mShape->mFileName, ShapeType::Collision, source.mShape->mFileHash, nextShapeId);
            default:
                Log(Debug::Warning) << "Trying to resolve recast mesh source with unsupported area type: " << source.mAreaType;
                assert(false);
                return ShapeId(0);
        }
    }
}
