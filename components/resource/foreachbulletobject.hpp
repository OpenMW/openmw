#ifndef OPENMW_COMPONENTS_RESOURCE_FOREACHBULLETOBJECT_H
#define OPENMW_COMPONENTS_RESOURCE_FOREACHBULLETOBJECT_H

#include <components/esm/defs.hpp>
#include <components/misc/convert.hpp>
#include <components/resource/bulletshape.hpp>

#include <osg/ref_ptr>

#include <functional>
#include <vector>

namespace ESM
{
    class ESMReader;
    struct Cell;
}

namespace VFS
{
    class Manager;
}

namespace Resource
{
    class BulletShapeManager;
}

namespace EsmLoader
{
    struct EsmData;
}

namespace Resource
{
    struct BulletObject
    {
        osg::ref_ptr<const Resource::BulletShape> mShape;
        ESM::Position mPosition;
        float mScale;
    };

    void forEachBulletObject(std::vector<ESM::ESMReader>& readers, const VFS::Manager& vfs,
        Resource::BulletShapeManager& bulletShapeManager, const EsmLoader::EsmData& esmData,
        std::function<void (const ESM::Cell&, const BulletObject& object)> callback);
}

#endif
