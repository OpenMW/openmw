#include "extra.hpp"

namespace Nif
{

    void NiExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);
        nif->readVector(data, recordSize);
    }

    void NiStringExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);
        string = nif->getString();
    }

    void NiTextKeyExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        int keynum = nif->getInt();
        list.resize(keynum);
        for (int i = 0; i < keynum; i++)
        {
            list[i].time = nif->getFloat();
            list[i].text = nif->getString();
        }
    }

    void NiVertWeightsExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->skip(nif->getUShort() * sizeof(float)); // vertex weights I guess
    }

    void NiIntegerExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        data = nif->getUInt();
    }

    void NiIntegersExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->readVector(data, nif->getUInt());
    }

    void NiBinaryExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);
        nif->readVector(data, nif->getUInt());
    }

    void NiBooleanExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);
        data = nif->getBoolean();
    }

    void NiVectorExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);
        data = nif->getVector4();
    }

    void NiFloatExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        data = nif->getFloat();
    }

    void NiFloatsExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);
        nif->readVector(data, nif->getUInt());
    }

    void BSBound::read(NIFStream* nif)
    {
        Extra::read(nif);
        center = nif->getVector3();
        halfExtents = nif->getVector3();
    }

    void BSFurnitureMarker::LegacyFurniturePosition::read(NIFStream* nif)
    {
        mOffset = nif->getVector3();
        mOrientation = nif->getUShort();
        mPositionRef = nif->getChar();
        nif->skip(1); // Position ref 2
    }

    void BSFurnitureMarker::FurniturePosition::read(NIFStream* nif)
    {
        mOffset = nif->getVector3();
        mHeading = nif->getFloat();
        mType = nif->getUShort();
        mEntryPoint = nif->getUShort();
    }

    void BSFurnitureMarker::read(NIFStream* nif)
    {
        Extra::read(nif);
        unsigned int num = nif->getUInt();
        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
        {
            mLegacyMarkers.resize(num);
            for (auto& marker : mLegacyMarkers)
                marker.read(nif);
        }
        else
        {
            mMarkers.resize(num);
            for (auto& marker : mMarkers)
                marker.read(nif);
        }
    }

    void BSInvMarker::read(NIFStream* nif)
    {
        Extra::read(nif);
        float rotX = nif->getUShort() / 1000.0;
        float rotY = nif->getUShort() / 1000.0;
        float rotZ = nif->getUShort() / 1000.0;
        mScale = nif->getFloat();

        mRotation = osg::Quat(rotX, osg::X_AXIS, rotY, osg::Y_AXIS, rotZ, osg::Z_AXIS);
    }

    void BSBehaviorGraphExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);
        mFile = nif->getString();
        mControlsBaseSkeleton = nif->getBoolean();
    }

}
