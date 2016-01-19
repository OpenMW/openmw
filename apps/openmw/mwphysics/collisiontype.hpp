#ifndef OPENMW_MWPHYSICS_COLLISIONTYPE_H
#define OPENMW_MWPHYSICS_COLLISIONTYPE_H

namespace MWPhysics
{

enum CollisionType {
    CollisionType_World = 1<<0,
    CollisionType_Door = 1<<1,
    CollisionType_Actor = 1<<2,
    CollisionType_HeightMap = 1<<3,
    CollisionType_Projectile = 1<<4,
    CollisionType_Water = 1<<5
};

}

#endif
