#ifndef OPENMW_MWPHYSICS_HEIGHTFIELD_H
#define OPENMW_MWPHYSICS_HEIGHTFIELD_H

#include <osg/ref_ptr>

#include <LinearMath/btScalar.h>

#include <memory>
#include <vector>

class btCollisionObject;
class btHeightfieldTerrainShape;

namespace osg
{
    class Object;
}

namespace ESMTerrain
{
    class LandObject;
}

namespace MWPhysics
{
    class PhysicsTaskScheduler;

    class HeightField
    {
    public:
        HeightField(const float* heights, int x, int y, int size, int verts, float minH, float maxH,
            std::shared_ptr<const ESMTerrain::LandObject> holdObject, PhysicsTaskScheduler* scheduler);
        ~HeightField();

        btCollisionObject* getCollisionObject();
        const btCollisionObject* getCollisionObject() const;
        const btHeightfieldTerrainShape* getShape() const;

    private:
        std::unique_ptr<btHeightfieldTerrainShape> mShape;
        std::unique_ptr<btCollisionObject> mCollisionObject;
        std::shared_ptr<const ESMTerrain::LandObject> mHoldObject;
#if BT_BULLET_VERSION < 310
        std::vector<btScalar> mHeights;
#endif

        PhysicsTaskScheduler* mTaskScheduler;

        void operator=(const HeightField&);
        HeightField(const HeightField&);
    };
}

#endif
