#include "physics.hpp"
#include "node.hpp"

namespace Nif
{
    void bhkCollisionObject::read(NIFStream *nif)
    {
        NiCollisionObject::read(nif);
        flags = nif->getUShort();
        body.read(nif);
    }

    void bhkWorldObject::read(NIFStream *nif)
    {
        shape.read(nif);
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->skip(4); // Unknown
        flags = nif->getUInt();
        nif->skip(4); // Unused
        worldObjectInfo.phaseType = nif->getChar();
        nif->skip(3); // Unused
        worldObjectInfo.data = nif->getUInt();
        worldObjectInfo.size = nif->getUInt();
        worldObjectInfo.capacityAndFlags = nif->getUInt();
    }

    void bhkWorldObject::post(NIFFile *nif)
    {
        shape.post(nif);
    }

    void bhkEntity::read(NIFStream *nif)
    {
        bhkWorldObject::read(nif);
        responseType = static_cast<hkResponseType>(nif->getChar());
        nif->skip(1); // Unused
        processContactDelay = nif->getUShort();
    }

    void HavokMaterial::read(NIFStream *nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->skip(4); // Unknown
        material = nif->getUInt();
    }

    void hkSubPartData::read(NIFStream *nif)
    {
        havokFilter = nif->getUInt();
        numVertices = nif->getUInt();
        material.read(nif);
    }

} // Namespace