#include "physics.hpp"
#include "node.hpp"

namespace Nif
{

    /// Non-record data types

    void bhkWorldObjCInfoProperty::read(NIFStream *nif)
    {
        mData = nif->getUInt();
        mSize = nif->getUInt();
        mCapacityAndFlags = nif->getUInt();
    }

    void bhkWorldObjectCInfo::read(NIFStream *nif)
    {
        nif->skip(4); // Unused
        mPhaseType = static_cast<BroadPhaseType>(nif->getChar());
        nif->skip(3); // Unused
        mProperty.read(nif);
    }

    void HavokMaterial::read(NIFStream *nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->skip(4); // Unknown
        mMaterial = nif->getUInt();
    }

    void HavokFilter::read(NIFStream *nif)
    {
        mLayer = nif->getChar();
        mFlags = nif->getChar();
        mGroup = nif->getUShort();
    }

    void hkSubPartData::read(NIFStream *nif)
    {
        mHavokFilter.read(nif);
        mNumVertices = nif->getUInt();
        mHavokMaterial.read(nif);
    }

    void bhkEntityCInfo::read(NIFStream *nif)
    {
        mResponseType = static_cast<hkResponseType>(nif->getChar());
        nif->skip(1); // Unused
        mProcessContactDelay = nif->getUShort();
    }

    /// Record types

    void bhkCollisionObject::read(NIFStream *nif)
    {
        NiCollisionObject::read(nif);
        mFlags = nif->getUShort();
        mBody.read(nif);
    }

    void bhkWorldObject::read(NIFStream *nif)
    {
        mShape.read(nif);
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->skip(4); // Unknown
        mHavokFilter.read(nif);
        mWorldObjectInfo.read(nif);
    }

    void bhkWorldObject::post(NIFFile *nif)
    {
        mShape.post(nif);
    }

    void bhkEntity::read(NIFStream *nif)
    {
        bhkWorldObject::read(nif);
        mInfo.read(nif);
    }

} // Namespace