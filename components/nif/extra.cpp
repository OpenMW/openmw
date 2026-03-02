#include "extra.hpp"

namespace Nif
{
    void NiExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->readVector(mData, mRecordSize);
    }

    void NiStringsExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->getSizedStrings(mData, nif->get<uint32_t>());
    }

    void NiTextKeyExtraData::TextKey::read(NIFStream* nif)
    {
        nif->read(mTime);
        nif->read(mText);
    }

    void NiTextKeyExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->readVectorOfRecords<uint32_t>(mList);
    }

    void NiVertWeightsExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->skip(nif->get<uint16_t>() * sizeof(float)); // vertex weights I guess
    }

    void BSBound::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->read(mCenter);
        nif->read(mExtents);
    }

    void BSFurnitureMarker::LegacyFurniturePosition::read(NIFStream* nif)
    {
        nif->read(mOffset);
        nif->read(mOrientation);
        nif->read(mPositionRef);
        nif->skip(1); // Position ref 2
    }

    void BSFurnitureMarker::FurniturePosition::read(NIFStream* nif)
    {
        nif->read(mOffset);
        nif->read(mHeading);
        nif->read(mType);
        nif->read(mEntryPoint);
    }

    void BSFurnitureMarker::read(NIFStream* nif)
    {
        Extra::read(nif);

        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
            nif->readVectorOfRecords<uint32_t>(mLegacyMarkers);
        else
            nif->readVectorOfRecords<uint32_t>(mMarkers);
    }

    void BSInvMarker::read(NIFStream* nif)
    {
        Extra::read(nif);

        float rotX = nif->get<uint16_t>() / 1000.f;
        float rotY = nif->get<uint16_t>() / 1000.f;
        float rotZ = nif->get<uint16_t>() / 1000.f;
        mRotation = osg::Quat(rotX, osg::X_AXIS, rotY, osg::Y_AXIS, rotZ, osg::Z_AXIS);
        nif->read(mScale);
    }

    void BSBehaviorGraphExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->read(mFile);
        nif->read(mControlsBaseSkeleton);
    }

    void BSBoneLODExtraData::read(NIFStream* nif)
    {
        Extra::read(nif);

        nif->readVectorOfRecords<uint32_t>(mData);
    }

    void BSBoneLODExtraData::BoneLOD::read(NIFStream* nif)
    {
        nif->read(mDistance);
        nif->read(mBone);
    }

    void BSDecalPlacementVectorExtraData::read(NIFStream* nif)
    {
        NiFloatExtraData::read(nif);

        nif->readVectorOfRecords<uint16_t>(mBlocks);
    }

    void BSDecalPlacementVectorExtraData::Block::read(NIFStream* nif)
    {
        nif->readVector(mPoints, nif->get<uint16_t>());
        nif->readVector(mNormals, mPoints.size());
    }

    void BSClothExtraData::read(NIFStream* nif)
    {
        nif->readVector(mData, nif->get<uint32_t>());
    }

    void BSCollisionQueryProxyExtraData::read(NIFStream* nif)
    {
        nif->readVector(mData, nif->get<uint32_t>());
    }

    void BSConnectPoint::Point::read(NIFStream* nif)
    {
        mParent = nif->getSizedString();
        mName = nif->getSizedString();
        nif->read(mTransform.mRotation);
        nif->read(mTransform.mTranslation);
        nif->read(mTransform.mScale);
    }

    void BSConnectPoint::Parents::read(NIFStream* nif)
    {
        NiExtraData::read(nif);

        nif->readVectorOfRecords<uint32_t>(mPoints);
    }

    void BSConnectPoint::Children::read(NIFStream* nif)
    {
        NiExtraData::read(nif);

        nif->read(mSkinned);
        nif->getSizedStrings(mPointNames, nif->get<uint32_t>());
    }

    void BSPackedGeomDataCombined::read(NIFStream* nif)
    {
        nif->read(mGrayscaleToPaletteScale);
        nif->read(mTransform);
        nif->read(mBoundingSphere);
    }

    void BSPackedGeomObject::read(NIFStream* nif)
    {
        nif->read(mFileHash);
        nif->read(mDataOffset);
    }

    void BSPackedSharedGeomData::read(NIFStream* nif)
    {
        nif->read(mNumVertices);
        nif->read(mLODLevels);
        nif->read(mLOD0TriCount);
        nif->read(mLOD0TriOffset);
        nif->read(mLOD1TriCount);
        nif->read(mLOD1TriOffset);
        nif->read(mLOD2TriCount);
        nif->read(mLOD2TriOffset);
        nif->readVectorOfRecords<uint32_t>(mCombined);
        mVertexDesc.read(nif);
    }

    void BSPackedCombinedSharedGeomDataExtra::read(NIFStream* nif)
    {
        NiExtraData::read(nif);

        mVertexDesc.read(nif);
        nif->read(mNumVertices);
        nif->read(mNumTriangles);
        nif->read(mFlags1);
        nif->read(mFlags2);
        nif->readVectorOfRecords<uint32_t>(mObjects);
        nif->readVectorOfRecords(mObjects.size(), mObjectData);
    }

}
