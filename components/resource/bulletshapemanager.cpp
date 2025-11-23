#include "bulletshapemanager.hpp"

#include <cstring>

#include <osg/Drawable>
#include <osg/NodeVisitor>
#include <osg/Transform>
#include <osg/TriangleFunctor>

#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include <components/misc/convert.hpp>
#include <components/misc/osguservalues.hpp>
#include <components/misc/pathhelpers.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

#include <components/nifbullet/bulletnifloader.hpp>

#include "bulletshape.hpp"
#include "multiobjectcache.hpp"
#include "niffilemanager.hpp"
#include "objectcache.hpp"
#include "scenemanager.hpp"

namespace Resource
{

    struct GetTriangleFunctor
    {
        GetTriangleFunctor()
            : mTriMesh(nullptr)
        {
        }

        void setTriMesh(btTriangleMesh* triMesh) { mTriMesh = triMesh; }

        void setMatrix(const osg::Matrixf& matrix) { mMatrix = matrix; }

        void inline operator()(const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3,
            bool /*temp*/ = false) // Note: unused temp argument left here for OSG versions less than 3.5.6
        {
            if (mTriMesh)
                mTriMesh->addTriangle(Misc::Convert::toBullet(mMatrix.preMult(v1)),
                    Misc::Convert::toBullet(mMatrix.preMult(v2)), Misc::Convert::toBullet(mMatrix.preMult(v3)));
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

        void apply(osg::Drawable& drawable) override
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
            if (!mTriangleMesh || mTriangleMesh->getNumTriangles() == 0)
                return osg::ref_ptr<BulletShape>();

            osg::ref_ptr<BulletShape> shape(new BulletShape);

            auto triangleMeshShape = std::make_unique<TriangleMeshShape>(mTriangleMesh.release(), true);
            btVector3 aabbMin = triangleMeshShape->getLocalAabbMin();
            btVector3 aabbMax = triangleMeshShape->getLocalAabbMax();
            shape->mCollisionBox.mExtents[0] = static_cast<float>(aabbMax[0] - aabbMin[0]) / 2.0f;
            shape->mCollisionBox.mExtents[1] = static_cast<float>(aabbMax[1] - aabbMin[1]) / 2.0f;
            shape->mCollisionBox.mExtents[2] = static_cast<float>(aabbMax[2] - aabbMin[2]) / 2.0f;
            shape->mCollisionBox.mCenter = osg::Vec3f(static_cast<float>(aabbMax[0] + aabbMin[0]) / 2.0f,
                static_cast<float>(aabbMax[1] + aabbMin[1]) / 2.0f, static_cast<float>(aabbMax[2] + aabbMin[2]) / 2.0f);
            shape->mCollisionShape.reset(triangleMeshShape.release());

            return shape;
        }

    private:
        std::unique_ptr<btTriangleMesh> mTriangleMesh;
    };

    BulletShapeManager::BulletShapeManager(
        const VFS::Manager* vfs, SceneManager* sceneMgr, NifFileManager* nifFileManager, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
        , mInstanceCache(new MultiObjectCache)
        , mSceneManager(sceneMgr)
        , mNifFileManager(nifFileManager)
    {
    }

    BulletShapeManager::~BulletShapeManager() = default;

    osg::ref_ptr<const BulletShape> BulletShapeManager::getShape(VFS::Path::NormalizedView name)
    {
        if (osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(name))
            return osg::ref_ptr<BulletShape>(static_cast<BulletShape*>(obj.get()));

        osg::ref_ptr<BulletShape> shape;

        if (Misc::getFileExtension(name.value()) == "nif")
        {
            NifBullet::BulletNifLoader loader;
            shape = loader.load(*mNifFileManager->get(name));
        }
        else
        {
            // TODO: support .bullet shape files

            osg::ref_ptr<const osg::Node> constNode(mSceneManager->getTemplate(name));
            // const-trickery required because there is no const version of NodeVisitor
            osg::ref_ptr<osg::Node> node(const_cast<osg::Node*>(constNode.get()));

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

            if (shape != nullptr)
            {
                shape->mFileName = name;
                constNode->getUserValue(Misc::OsgUserValues::sFileHash, shape->mFileHash);
            }
        }

        mCache->addEntryToObjectCache(name.value(), shape);

        return shape;
    }

    osg::ref_ptr<BulletShapeInstance> BulletShapeManager::cacheInstance(VFS::Path::NormalizedView name)
    {
        osg::ref_ptr<BulletShapeInstance> instance = createInstance(name);
        if (instance != nullptr)
            mInstanceCache->addEntryToObjectCache(name, instance.get());
        return instance;
    }

    osg::ref_ptr<BulletShapeInstance> BulletShapeManager::getInstance(VFS::Path::NormalizedView name)
    {
        if (osg::ref_ptr<osg::Object> obj = mInstanceCache->takeFromObjectCache(name))
            return static_cast<BulletShapeInstance*>(obj.get());
        return createInstance(name);
    }

    osg::ref_ptr<BulletShapeInstance> BulletShapeManager::createInstance(VFS::Path::NormalizedView name)
    {
        if (osg::ref_ptr<const BulletShape> shape = getShape(name))
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

    void BulletShapeManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("Shape", frameNumber, mCache->getStats(), *stats);
        Resource::reportStats("Shape Instance", frameNumber, mInstanceCache->getStats(), *stats);
    }

}
