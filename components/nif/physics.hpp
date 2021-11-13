#ifndef OPENMW_COMPONENTS_NIF_PHYSICS_HPP
#define OPENMW_COMPONENTS_NIF_PHYSICS_HPP

#include "base.hpp"

// This header contains certain record definitions
// specific to Bethesda implementation of Havok physics
namespace Nif
{

/// Non-record data types

struct bhkWorldObjCInfoProperty
{
    unsigned int mData;
    unsigned int mSize;
    unsigned int mCapacityAndFlags;
    void read(NIFStream *nif);
};

enum class BroadPhaseType : uint8_t
{
    BroadPhase_Invalid = 0,
    BroadPhase_Entity = 1,
    BroadPhase_Phantom = 2,
    BroadPhase_Border = 3
};

struct bhkWorldObjectCInfo
{
    BroadPhaseType mPhaseType;
    bhkWorldObjCInfoProperty mProperty;
    void read(NIFStream *nif);
};

struct HavokMaterial
{
    unsigned int mMaterial;
    void read(NIFStream *nif);
};

struct HavokFilter
{
    unsigned char mLayer;
    unsigned char mFlags;
    unsigned short mGroup;
    void read(NIFStream *nif);
};

struct hkSubPartData
{
    HavokMaterial mHavokMaterial;
    unsigned int mNumVertices;
    HavokFilter mHavokFilter;
    void read(NIFStream *nif);
};

enum class hkResponseType : uint8_t
{
    Response_Invalid = 0,
    Response_SimpleContact = 1,
    Response_Reporting = 2,
    Response_None = 3
};

struct bhkEntityCInfo
{
    hkResponseType mResponseType;
    unsigned short mProcessContactDelay;
    void read(NIFStream *nif);
}

struct hkpMoppCode
{
    osg::Vec4f mOffset;
    std::vector<char> mData;
    void read(NIFStream *nif);
};

struct TriangleData
{
    unsigned short mTriangle[3];
    unsigned short mWeldingInfo;
    osg::Vec3f mNormal;
    void read(NIFStream *nif);
};

/// Record types

// Abstract Bethesda Havok object
struct bhkRefObject : public Record {};

// Abstract serializable Bethesda Havok object
struct bhkSerializable : public bhkRefObject {};

// Abstract narrowphase collision detection object
struct bhkShape : public bhkSerializable {};

// Abstract bhkShape collection
struct bhkShapeCollection : public bhkShape {};

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
    bhkWorldObjectPtr mBody;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override
    {
        NiCollisionObject::post(nif);
        mBody.post(nif);
    }
};

// Abstract Havok shape info record
struct bhkWorldObject : public bhkSerializable
{
    bhkShapePtr mShape;
    HavokFilter mHavokFilter;
    bhkWorldObjectCInfo mWorldObjectInfo;
    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

// Abstract
struct bhkEntity : public bhkWorldObject
{
    bhkEntityCInfo mInfo;
    void read(NIFStream *nif) override;
};

// Bethesda extension of hkpBvTreeShape
// hkpBvTreeShape adds a bounding volume tree to an hkpShapeCollection
struct bhkBvTreeShape : public bhkShape
{
    bhkShapePtr mShape;
    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

// bhkBvTreeShape with Havok MOPP code
struct bhkMoppBvTreeShape : public bhkBvTreeShape
{
    float mScale;
    hkpMoppCode mMopp;
    void read(NIFStream *nif) override;
};

// Bethesda triangle strip-based Havok shape collection
struct bhkNiTriStripsShape : public bhkShape
{
    HavokMaterial mHavokMaterial;
    float mRadius;
    unsigned int mGrowBy;
    osg::Vec4f mScale{1.f, 1.f, 1.f, 0.f};
    NiTriStripsDataList mData;
    std::vector<unsigned int> mFilters;
    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

// Bethesda packed triangle strip-based Havok shape collection
struct bhkPackedNiTriStripsShape : public bhkShapeCollection
{
    std::vector<hkSubPartData> mSubshapes;
    unsigned int mUserData;
    float mRadius;
    osg::Vec4f mScale;
    hkPackedNiTriStripsDataPtr mData;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

// bhkPackedNiTriStripsShape data block
struct hkPackedNiTriStripsData : public bhkShapeCollection
{
    std::vector<TriangleData> mTriangles;
    std::vector<osg::Vec3f> mVertices;
    std::vector<hkSubPartData> mSubshapes;
    void read(NIFStream *nif) override;
};

} // Namespace
#endif