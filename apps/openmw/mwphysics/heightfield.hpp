#ifndef OPENMW_MWPHYSICS_HEIGHTFIELD_H
#define OPENMW_MWPHYSICS_HEIGHTFIELD_H

#include <osg/ref_ptr>

class btCollisionObject;
class btHeightfieldTerrainShape;

namespace osg
{
    class Object;
}

namespace MWPhysics
{
    class HeightField
    {
    public:
        HeightField(const float* heights, int x, int y, float triSize, float sqrtVerts, float minH, float maxH, const osg::Object* holdObject);
        ~HeightField();

        btCollisionObject* getCollisionObject();
        const btCollisionObject* getCollisionObject() const;
        const btHeightfieldTerrainShape* getShape() const;

    private:
        btHeightfieldTerrainShape* mShape;
        btCollisionObject* mCollisionObject;
        osg::ref_ptr<const osg::Object> mHoldObject;

        void operator=(const HeightField&);
        HeightField(const HeightField&);
    };
}

#endif
