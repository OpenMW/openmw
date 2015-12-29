#ifndef OPENMW_COMPONENTS_RESOURCE_BULLETSHAPE_H
#define OPENMW_COMPONENTS_RESOURCE_BULLETSHAPE_H

#include <map>

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <osg/Vec3f>

#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>

class btCollisionShape;

namespace Resource
{

    class BulletShapeInstance;
    class BulletShape : public osg::Referenced
    {
    public:
        BulletShape();
        virtual ~BulletShape();

        btCollisionShape* mCollisionShape;

        // Used for actors. Note, ideally actors would use a separate loader - as it is
        // we have to keep a redundant copy of the actor model around in mCollisionShape, which isn't used.
        // For now, use one file <-> one resource for simplicity.
        osg::Vec3f mCollisionBoxHalfExtents;
        osg::Vec3f mCollisionBoxTranslate;

        // Stores animated collision shapes. If any collision nodes in the NIF are animated, then mCollisionShape
        // will be a btCompoundShape (which consists of one or more child shapes).
        // In this map, for each animated collision shape,
        // we store the node's record index mapped to the child index of the shape in the btCompoundShape.
        std::map<int, int> mAnimatedShapes;

        osg::ref_ptr<BulletShapeInstance> makeInstance();

        btCollisionShape* duplicateCollisionShape(btCollisionShape* shape) const;

        btCollisionShape* getCollisionShape();

    private:
        void deleteShape(btCollisionShape* shape);
    };


    // An instance of a BulletShape that may have its own unique scaling set on the mCollisionShape.
    // Vertex data is shallow-copied where possible. A ref_ptr to the original shape is held to keep vertex pointers intact.
    class BulletShapeInstance : public BulletShape
    {
    public:
        BulletShapeInstance(osg::ref_ptr<BulletShape> source);

    private:
        osg::ref_ptr<BulletShape> mSource;
    };

    // Subclass btBhvTriangleMeshShape to auto-delete the meshInterface
    struct TriangleMeshShape : public btBvhTriangleMeshShape
    {
        TriangleMeshShape(btStridingMeshInterface* meshInterface, bool useQuantizedAabbCompression, bool buildBvh = true)
            : btBvhTriangleMeshShape(meshInterface, useQuantizedAabbCompression, buildBvh)
        {
        }

        virtual ~TriangleMeshShape()
        {
            delete getTriangleInfoMap();
            delete m_meshInterface;
        }
    };


}

#endif
