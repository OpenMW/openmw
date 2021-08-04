#ifndef OPENMW_MWPHYSICS_HEIGHTFIELD_H
#define OPENMW_MWPHYSICS_HEIGHTFIELD_H

#include <osg/ref_ptr>
#include <osg/Object>

#include <LinearMath/btScalar.h>

#include <memory>
#include <vector>

class btCollisionObject;
class btHeightfieldTerrainShape;

namespace osg
{
    class Object;
}

namespace MWPhysics
{
    class PhysicsTaskScheduler;

    class HeightField : public osg::Object
    {
    public:
        HeightField(const float* heights, int x, int y, float triSize, float sqrtVerts, float minH, float maxH, const osg::Object* holdObject, PhysicsTaskScheduler* scheduler);
        ~HeightField();

        META_Object(MWPhysics, HeightField)

        btCollisionObject* getCollisionObject();
        const btCollisionObject* getCollisionObject() const;
        const btHeightfieldTerrainShape* getShape() const;

    private:
        std::unique_ptr<btHeightfieldTerrainShape> mShape;
        std::unique_ptr<btCollisionObject> mCollisionObject;
        osg::ref_ptr<const osg::Object> mHoldObject;
#if BT_BULLET_VERSION < 310
        std::vector<btScalar> mHeights;
#endif

        PhysicsTaskScheduler* mTaskScheduler;

        HeightField();
        HeightField(const HeightField&, const osg::CopyOp&);
        void operator=(const HeightField&);
        HeightField(const HeightField&);
    };
}

#endif
