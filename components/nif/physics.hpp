#ifndef OPENMW_COMPONENTS_NIF_PHYSICS_HPP
#define OPENMW_COMPONENTS_NIF_PHYSICS_HPP

#include "base.hpp"

// This header contains certain record definitions
// specific to Bethesda implementation of Havok physics
namespace Nif
{

// Generic collision object
struct NiCollisionObject : public Record
{
    // The node that references this object
    NodePtr mTarget;

    void read(NIFStream *nif) override
    {
        mTarget.read(nif);
    }
    void post(NIFFile *nif) override
    {
        mTarget.post(nif);
    }
};

// Bethesda Havok-specific collision object
struct bhkCollisionObject : public NiCollisionObject
{
    unsigned short mFlags;
    CollisionBodyPtr mBody;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override
    {
        NiCollisionObject::post(nif);
        mBody.post(nif);
    }
};

// Abstract Havok shape info record
struct bhkWorldObject : public Record
{
    bhkShapePtr mShape;
    unsigned int mFlags; // Havok layer type, collision filter flags and group
    struct WorldObjectInfo
    {
        unsigned char mPhaseType;
        unsigned int mData;
        unsigned int mSize;
        unsigned int mCapacityAndFlags;
    };
    WorldObjectInfo mWorldObjectInfo;
    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct bhkShape : public Record {};

enum class hkResponseType : uint8_t
{
    Response_Invalid = 0,
    Response_SimpleContact = 1,
    Response_Reporting = 2,
    Response_None = 3
};

struct bhkEntity : public bhkWorldObject
{
    hkResponseType mResponseType;
    unsigned short mProcessContactDelay;
    void read(NIFStream *nif) override;
};

struct HavokMaterial
{
    unsigned int mMaterial;
    void read(NIFStream *nif);
};

struct hkSubPartData
{
    HavokMaterial mHavokMaterial;
    unsigned int mNumVertices;
    unsigned int mHavokFilter;
    void read(NIFStream *nif);
};

} // Namespace
#endif