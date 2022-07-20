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
    CollisionType_Water = 1<<5,
    CollisionType_Default = CollisionType_World|CollisionType_HeightMap|CollisionType_Actor|CollisionType_Door,
    CollisionType_AnyPhysical = CollisionType_World|CollisionType_HeightMap|CollisionType_Actor|CollisionType_Door|CollisionType_Projectile|CollisionType_Water,
    CollisionType_CameraOnly = 1<<6,
    CollisionType_VisualOnly = 1<<7
};

}

#endif
