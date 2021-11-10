#include "physics.hpp"
#include "node.hpp"

namespace Nif
{
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
        mFlags = nif->getUInt();
        nif->skip(4); // Unused
        mWorldObjectInfo.mPhaseType = nif->getChar();
        nif->skip(3); // Unused
        mWorldObjectInfo.mData = nif->getUInt();
        mWorldObjectInfo.mSize = nif->getUInt();
        mWorldObjectInfo.mCapacityAndFlags = nif->getUInt();
    }

    void bhkWorldObject::post(NIFFile *nif)
    {
        mShape.post(nif);
    }

    void bhkEntity::read(NIFStream *nif)
    {
        bhkWorldObject::read(nif);
        mResponseType = static_cast<hkResponseType>(nif->getChar());
        nif->skip(1); // Unused
        mProcessContactDelay = nif->getUShort();
    }

    void HavokMaterial::read(NIFStream *nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->skip(4); // Unknown
        mMaterial = nif->getUInt();
    }

    void hkSubPartData::read(NIFStream *nif)
    {
        mHavokFilter = nif->getUInt();
        mNumVertices = nif->getUInt();
        mHavokMaterial.read(nif);
    }

} // Namespace