#ifndef OPENMW_COMPONENTS_RESOURCE_BULLETSHAPE_H
#define OPENMW_COMPONENTS_RESOURCE_BULLETSHAPE_H

#include <map>
#include <memory>

#include <osg/Object>
#include <osg/Vec3f>
#include <osg/ref_ptr>

#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h>

#include <components/vfs/pathutil.hpp>

class btCollisionShape;

namespace NifBullet
{
    class BulletNifLoader;
}

namespace Resource
{
    struct DeleteCollisionShape
    {
        void operator()(btCollisionShape* shape) const;
    };

    using CollisionShapePtr = std::unique_ptr<btCollisionShape, DeleteCollisionShape>;

    struct CollisionBox
    {
        osg::Vec3f mExtents;
        osg::Vec3f mCenter;
    };

    enum class VisualCollisionType
    {
        None,
        Default,
        Camera
    };

    struct BulletShape : public osg::Object
    {
        CollisionShapePtr mCollisionShape;
        CollisionShapePtr mAvoidCollisionShape;

        // Used for actors and projectiles. mCollisionShape is used for actors only when we need to autogenerate
        // collision box for creatures. For now, use one file <-> one resource for simplicity.
        CollisionBox mCollisionBox;

        // Stores animated collision shapes.
        // mCollisionShape is a btCompoundShape (which consists of one or more child shapes).
        // In this map, for each animated collision shape,
        // we store the node's record index mapped to the child index of the shape in the btCompoundShape.
        std::map<int, int> mAnimatedShapes;

        VFS::Path::Normalized mFileName;
        std::string mFileHash;

        VisualCollisionType mVisualCollisionType = VisualCollisionType::None;

        BulletShape() = default;
        // Note this is always a shallow copy and the copy will not autodelete underlying vertex data
        BulletShape(const BulletShape& other, const osg::CopyOp& copyOp = osg::CopyOp());

        META_Object(Resource, BulletShape)

        void setLocalScaling(const btVector3& scale);

        bool isAnimated() const { return !mAnimatedShapes.empty(); }
    };

    // An instance of a BulletShape that may have its own unique scaling set on collision shapes.
    // Vertex data is shallow-copied where possible. A ref_ptr to the original shape is held to keep vertex pointers
    // intact.
    class BulletShapeInstance : public BulletShape
    {
    public:
        explicit BulletShapeInstance(osg::ref_ptr<const BulletShape> source);

        const osg::ref_ptr<const BulletShape>& getSource() const { return mSource; }

    private:
        osg::ref_ptr<const BulletShape> mSource;
    };

    osg::ref_ptr<BulletShapeInstance> makeInstance(osg::ref_ptr<const BulletShape> source);

    // Subclass btBhvTriangleMeshShape to auto-delete the meshInterface
    struct TriangleMeshShape : public btBvhTriangleMeshShape
    {
        TriangleMeshShape(
            btStridingMeshInterface* meshInterface, bool useQuantizedAabbCompression, bool buildBvh = true)
            : btBvhTriangleMeshShape(meshInterface, useQuantizedAabbCompression, buildBvh)
        {
        }

        virtual ~TriangleMeshShape()
        {
            delete getTriangleInfoMap();
            delete m_meshInterface;
        }
    };

    // btScaledBvhTriangleMeshShape that auto-deletes the child shape
    struct ScaledTriangleMeshShape : public btScaledBvhTriangleMeshShape
    {
        ScaledTriangleMeshShape(btBvhTriangleMeshShape* childShape, const btVector3& localScaling)
            : btScaledBvhTriangleMeshShape(childShape, localScaling)
        {
        }

        ~ScaledTriangleMeshShape() override { delete getChildShape(); }
    };

}

#endif
