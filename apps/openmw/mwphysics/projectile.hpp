#ifndef OPENMW_MWPHYSICS_PROJECTILE_H
#define OPENMW_MWPHYSICS_PROJECTILE_H

#include <memory>

#include "ptrholder.hpp"

#include <osg/Vec3f>
#include <osg/Quat>
#include <osg/ref_ptr>

class btCollisionWorld;
class btCollisionShape;
class btCollisionObject;
class btConvexShape;

namespace Resource
{
    class BulletShape;
}

namespace MWPhysics
{
    class Projectile : public PtrHolder
    {
    public:
        Projectile(const int projectileId, const osg::Vec3f& position, btCollisionWorld* world);
        ~Projectile();

        btConvexShape* getConvexShape() const { return mConvexShape; }

        void updateCollisionObjectPosition();

        void setPosition(const osg::Vec3f& position);

        osg::Vec3f getPosition() const;

        btCollisionObject* getCollisionObject() const
        {
            return mCollisionObject.get();
        }

        int getProjectileId() const
        {
            return mProjectileId;
        }

    private:

        std::unique_ptr<btCollisionShape> mShape;
        btConvexShape* mConvexShape;

        std::unique_ptr<btCollisionObject> mCollisionObject;

        osg::Vec3f mPosition;

        btCollisionWorld* mCollisionWorld;

        Projectile(const Projectile&);
        Projectile& operator=(const Projectile&);

        int mProjectileId;
    };

}


#endif
