#include "bulletshapemanager.hpp"

#include <osg/NodeVisitor>
#include <osg/TriangleFunctor>
#include <osg/Transform>
#include <osg/Drawable>

#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include <components/misc/pathhelpers.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/vfs/manager.hpp>

#include <components/nifbullet/bulletnifloader.hpp>

#include "bulletshape.hpp"
#include "scenemanager.hpp"
#include "niffilemanager.hpp"
#include "objectcache.hpp"
#include "multiobjectcache.hpp"

namespace Resource
{

struct GetTriangleFunctor
{
    GetTriangleFunctor()
        : mTriMesh(nullptr)
    {
    }

    void setTriMesh(btTriangleMesh* triMesh)
    {
        mTriMesh = triMesh;
    }

    void setMatrix(const osg::Matrixf& matrix)
    {
        mMatrix = matrix;
    }

    inline btVector3 toBullet(const osg::Vec3f& vec)
    {
        return btVector3(vec.x(), vec.y(), vec.z());
    }

    void inline operator()( const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3, bool _temp=false ) // Note: unused temp argument left here for OSG versions less than 3.5.6 
    {
        if (mTriMesh)
            mTriMesh->addTriangle( toBullet(mMatrix.preMult(v1)), toBullet(mMatrix.preMult(v2)), toBullet(mMatrix.preMult(v3)));
    }

    btTriangleMesh* mTriMesh;
    osg::Matrixf mMatrix;
};

/// Creates a BulletShape out of a Node hierarchy.
class NodeToShapeVisitor : public osg::NodeVisitor
{
public:
    NodeToShapeVisitor()
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mTriangleMesh(nullptr)
    {

    }

    void apply(osg::Drawable &drawable) override
    {
        if (!mTriangleMesh)
            mTriangleMesh.reset(new btTriangleMesh);

        osg::Matrixf worldMat = osg::computeLocalToWorld(getNodePath());
        osg::TriangleFunctor<GetTriangleFunctor> functor;
        functor.setTriMesh(mTriangleMesh.get());
        functor.setMatrix(worldMat);
        drawable.accept(functor);
    }

    osg::ref_ptr<BulletShape> getShape()
    {
        if (!mTriangleMesh)
            return osg::ref_ptr<BulletShape>();

        osg::ref_ptr<BulletShape> shape (new BulletShape);

        auto triangleMeshShape = std::make_unique<TriangleMeshShape>(mTriangleMesh.release(), true);
        btVector3 aabbMin = triangleMeshShape->getLocalAabbMin();
        btVector3 aabbMax = triangleMeshShape->getLocalAabbMax();
        shape->mCollisionBox.mExtents[0] = (aabbMax[0] - aabbMin[0]) / 2.0f;
        shape->mCollisionBox.mExtents[1] = (aabbMax[1] - aabbMin[1]) / 2.0f;
        shape->mCollisionBox.mExtents[2] = (aabbMax[2] - aabbMin[2]) / 2.0f;
        shape->mCollisionBox.mCenter = osg::Vec3f( (aabbMax[0] + aabbMin[0]) / 2.0f,
                                                  (aabbMax[1] + aabbMin[1]) / 2.0f,
                                                  (aabbMax[2] + aabbMin[2]) / 2.0f );
        shape->mCollisionShape.reset(triangleMeshShape.release());

        return shape;
    }

private:
    std::unique_ptr<btTriangleMesh> mTriangleMesh;
};

BulletShapeManager::BulletShapeManager(const VFS::Manager* vfs, SceneManager* sceneMgr, NifFileManager* nifFileManager)
    : ResourceManager(vfs)
    , mInstanceCache(new MultiObjectCache)
    , mSceneManager(sceneMgr)
    , mNifFileManager(nifFileManager)
{

}

BulletShapeManager::~BulletShapeManager()
{

}

osg::ref_ptr<const BulletShape> BulletShapeManager::getShape(const std::string &name)
{
    const std::string normalized = mVFS->normalizeFilename(name);

    osg::ref_ptr<BulletShape> shape;
    osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
    if (obj)
        shape = osg::ref_ptr<BulletShape>(static_cast<BulletShape*>(obj.get()));
    else
    {
        if (Misc::getFileExtension(normalized) == "nif")
        {
            NifBullet::BulletNifLoader loader;
            shape = loader.load(*mNifFileManager->get(normalized));
        }
        else
        {
            // TODO: support .bullet shape files

            osg::ref_ptr<const osg::Node> constNode (mSceneManager->getTemplate(normalized));
            osg::ref_ptr<osg::Node> node (const_cast<osg::Node*>(constNode.get())); // const-trickery required because there is no const version of NodeVisitor

            // Check first if there's a custom collision node
            unsigned int visitAllNodesMask = 0xffffffff;
            SceneUtil::FindByNameVisitor nameFinder("Collision");
            nameFinder.setTraversalMask(visitAllNodesMask);
            nameFinder.setNodeMaskOverride(visitAllNodesMask);
            node->accept(nameFinder);
            if (nameFinder.mFoundNode)
            {
                NodeToShapeVisitor visitor;
                visitor.setTraversalMask(visitAllNodesMask);
                visitor.setNodeMaskOverride(visitAllNodesMask);
                nameFinder.mFoundNode->accept(visitor);
                shape = visitor.getShape();
            }

            // Generate a collision shape from the mesh
            if (!shape)
            {
                NodeToShapeVisitor visitor;
                node->accept(visitor);
                shape = visitor.getShape();
                if (!shape)
                    return osg::ref_ptr<BulletShape>();
            }
        }

        mCache->addEntryToObjectCache(normalized, shape);
    }
    return shape;
}

osg::ref_ptr<BulletShapeInstance> BulletShapeManager::cacheInstance(const std::string &name)
{
    const std::string normalized = mVFS->normalizeFilename(name);

    osg::ref_ptr<BulletShapeInstance> instance = createInstance(normalized);
    if (instance)
        mInstanceCache->addEntryToObjectCache(normalized, instance.get());
    return instance;
}

osg::ref_ptr<BulletShapeInstance> BulletShapeManager::getInstance(const std::string &name)
{
    const std::string normalized = mVFS->normalizeFilename(name);

    osg::ref_ptr<osg::Object> obj = mInstanceCache->takeFromObjectCache(normalized);
    if (obj.get())
        return static_cast<BulletShapeInstance*>(obj.get());
    else
        return createInstance(normalized);
}

osg::ref_ptr<BulletShapeInstance> BulletShapeManager::createInstance(const std::string &name)
{
    osg::ref_ptr<const BulletShape> shape = getShape(name);
    if (shape)
        return makeInstance(std::move(shape));
    return osg::ref_ptr<BulletShapeInstance>();
}

void BulletShapeManager::updateCache(double referenceTime)
{
    ResourceManager::updateCache(referenceTime);

    mInstanceCache->removeUnreferencedObjectsInCache();
}

void BulletShapeManager::clearCache()
{
    ResourceManager::clearCache();

    mInstanceCache->clear();
}

void BulletShapeManager::reportStats(unsigned int frameNumber, osg::Stats *stats) const
{
    stats->setAttribute(frameNumber, "Shape", mCache->getCacheSize());
    stats->setAttribute(frameNumber, "Shape Instance", mInstanceCache->getCacheSize());
}

}
