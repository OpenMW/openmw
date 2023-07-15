#include "physics.hpp"

#include "data.hpp"
#include "node.hpp"

namespace Nif
{

    /// Non-record data types

    void bhkWorldObjCInfoProperty::read(NIFStream* nif)
    {
        mData = nif->getUInt();
        mSize = nif->getUInt();
        mCapacityAndFlags = nif->getUInt();
    }

    void bhkWorldObjectCInfo::read(NIFStream* nif)
    {
        nif->skip(4); // Unused
        mPhaseType = static_cast<BroadPhaseType>(nif->getChar());
        nif->skip(3); // Unused
        mProperty.read(nif);
    }

    void HavokMaterial::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->skip(4); // Unknown
        mMaterial = nif->getUInt();
    }

    void HavokFilter::read(NIFStream* nif)
    {
        mLayer = nif->getChar();
        mFlags = nif->getChar();
        mGroup = nif->getUShort();
    }

    void hkSubPartData::read(NIFStream* nif)
    {
        mHavokFilter.read(nif);
        mNumVertices = nif->getUInt();
        mHavokMaterial.read(nif);
    }

    void hkpMoppCode::read(NIFStream* nif)
    {
        unsigned int size = nif->getUInt();
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            mOffset = nif->getVector4();
        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
            nif->getChar(); // MOPP data build type
        if (size)
            nif->getChars(mData, size);
    }

    void bhkEntityCInfo::read(NIFStream* nif)
    {
        mResponseType = static_cast<hkResponseType>(nif->getChar());
        nif->skip(1); // Unused
        mProcessContactDelay = nif->getUShort();
    }

    void TriangleData::read(NIFStream* nif)
    {
        for (int i = 0; i < 3; i++)
            mTriangle[i] = nif->getUShort();
        mWeldingInfo = nif->getUShort();
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
            mNormal = nif->getVector3();
    }

    void bhkMeshMaterial::read(NIFStream* nif)
    {
        mHavokMaterial.read(nif);
        mHavokFilter.read(nif);
    }

    void bhkQsTransform::read(NIFStream* nif)
    {
        mTranslation = nif->getVector4();
        mRotation = nif->getQuaternion();
    }

    void bhkCMSBigTri::read(NIFStream* nif)
    {
        for (int i = 0; i < 3; i++)
            mTriangle[i] = nif->getUShort();
        mMaterial = nif->getUInt();
        mWeldingInfo = nif->getUShort();
    }

    void bhkCMSChunk::read(NIFStream* nif)
    {
        mTranslation = nif->getVector4();
        mMaterialIndex = nif->getUInt();
        mReference = nif->getUShort();
        mTransformIndex = nif->getUShort();
        size_t numVertices = nif->getUInt();
        if (numVertices)
            nif->getUShorts(mVertices, numVertices);
        size_t numIndices = nif->getUInt();
        if (numIndices)
            nif->getUShorts(mIndices, numIndices);
        size_t numStrips = nif->getUInt();
        if (numStrips)
            nif->getUShorts(mStrips, numStrips);
        size_t numInfos = nif->getUInt();
        if (numInfos)
            nif->getUShorts(mWeldingInfos, numInfos);
    }

    void bhkRigidBodyCInfo::read(NIFStream* nif)
    {
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            nif->skip(4); // Unused
            mHavokFilter.read(nif);
            nif->skip(4); // Unused
            if (nif->getBethVersion() != NIFFile::BethVersion::BETHVER_FO4)
            {
                if (nif->getBethVersion() >= 83)
                    nif->skip(4); // Unused
                mResponseType = static_cast<hkResponseType>(nif->getChar());
                nif->skip(1); // Unused
                mProcessContactDelay = nif->getUShort();
            }
        }
        if (nif->getBethVersion() < 83)
            nif->skip(4); // Unused
        mTranslation = nif->getVector4();
        mRotation = nif->getQuaternion();
        mLinearVelocity = nif->getVector4();
        mAngularVelocity = nif->getVector4();
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 4; j++)
                mInertiaTensor[i][j] = nif->getFloat();
        mCenter = nif->getVector4();
        mMass = nif->getFloat();
        mLinearDamping = nif->getFloat();
        mAngularDamping = nif->getFloat();
        if (nif->getBethVersion() >= 83)
        {
            if (nif->getBethVersion() != NIFFile::BethVersion::BETHVER_FO4)
                mTimeFactor = nif->getFloat();
            mGravityFactor = nif->getFloat();
        }
        mFriction = nif->getFloat();
        if (nif->getBethVersion() >= 83)
            mRollingFrictionMult = nif->getFloat();
        mRestitution = nif->getFloat();
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            mMaxLinearVelocity = nif->getFloat();
            mMaxAngularVelocity = nif->getFloat();
            if (nif->getBethVersion() != NIFFile::BethVersion::BETHVER_FO4)
                mPenetrationDepth = nif->getFloat();
        }
        mMotionType = static_cast<hkMotionType>(nif->getChar());
        if (nif->getBethVersion() < 83)
            mDeactivatorType = static_cast<hkDeactivatorType>(nif->getChar());
        else
            mEnableDeactivation = nif->getBoolean();
        mSolverDeactivation = static_cast<hkSolverDeactivation>(nif->getChar());
        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_FO4)
        {
            nif->skip(1);
            mPenetrationDepth = nif->getFloat();
            mTimeFactor = nif->getFloat();
            nif->skip(4);
            mResponseType = static_cast<hkResponseType>(nif->getChar());
            nif->skip(1); // Unused
            mProcessContactDelay = nif->getUShort();
        }
        mQualityType = static_cast<hkQualityType>(nif->getChar());
        if (nif->getBethVersion() >= 83)
        {
            mAutoRemoveLevel = nif->getChar();
            mResponseModifierFlags = nif->getChar();
            mNumContactPointShapeKeys = nif->getChar();
            mForceCollidedOntoPPU = nif->getBoolean();
        }
        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_FO4)
            nif->skip(3); // Unused
        else
            nif->skip(12); // Unused
    }

    void bhkConstraintCInfo::read(NIFStream* nif)
    {
        nif->get<unsigned int>(); // Number of entities, unused
        mEntities.resize(2); // Hardcoded
        for (auto& entity : mEntities)
            entity.read(nif);

        mPriority = static_cast<ConstraintPriority>(nif->get<uint32_t>());
    }

    void bhkConstraintCInfo::post(Reader& nif)
    {
        postRecordList(nif, mEntities);
    }

    /// Record types

    void bhkCollisionObject::read(NIFStream* nif)
    {
        NiCollisionObject::read(nif);
        mFlags = nif->getUShort();
        mBody.read(nif);
    }

    void bhkWorldObject::read(NIFStream* nif)
    {
        mShape.read(nif);
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->skip(4); // Unknown
        mHavokFilter.read(nif);
        mWorldObjectInfo.read(nif);
    }

    void bhkWorldObject::post(Reader& nif)
    {
        mShape.post(nif);
    }

    void bhkEntity::read(NIFStream* nif)
    {
        bhkWorldObject::read(nif);
        mInfo.read(nif);
    }

    void bhkBvTreeShape::read(NIFStream* nif)
    {
        mShape.read(nif);
    }

    void bhkBvTreeShape::post(Reader& nif)
    {
        mShape.post(nif);
    }

    void bhkMoppBvTreeShape::read(NIFStream* nif)
    {
        bhkBvTreeShape::read(nif);
        nif->skip(12); // Unused
        mScale = nif->getFloat();
        mMopp.read(nif);
    }

    void bhkNiTriStripsShape::read(NIFStream* nif)
    {
        mHavokMaterial.read(nif);
        mRadius = nif->getFloat();
        nif->skip(20); // Unused
        mGrowBy = nif->getUInt();
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            mScale = nif->getVector4();
        readRecordList(nif, mData);
        unsigned int numFilters = nif->getUInt();
        nif->getUInts(mFilters, numFilters);
    }

    void bhkNiTriStripsShape::post(Reader& nif)
    {
        postRecordList(nif, mData);
    }

    void bhkPackedNiTriStripsShape::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            mSubshapes.resize(nif->getUShort());
            for (hkSubPartData& subshape : mSubshapes)
                subshape.read(nif);
        }
        mUserData = nif->getUInt();
        nif->skip(4); // Unused
        mRadius = nif->getFloat();
        nif->skip(4); // Unused
        mScale = nif->getVector4();
        nif->skip(20); // Duplicates of the two previous fields
        mData.read(nif);
    }

    void bhkPackedNiTriStripsShape::post(Reader& nif)
    {
        mData.post(nif);
    }

    void hkPackedNiTriStripsData::read(NIFStream* nif)
    {
        unsigned int numTriangles = nif->getUInt();
        mTriangles.resize(numTriangles);
        for (unsigned int i = 0; i < numTriangles; i++)
            mTriangles[i].read(nif);

        unsigned int numVertices = nif->getUInt();
        bool compressed = false;
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS)
            compressed = nif->getBoolean();
        if (!compressed)
            nif->getVector3s(mVertices, numVertices);
        else
            nif->skip(6 * numVertices); // Half-precision vectors are not currently supported
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS)
        {
            mSubshapes.resize(nif->getUShort());
            for (hkSubPartData& subshape : mSubshapes)
                subshape.read(nif);
        }
    }

    void bhkSphereRepShape::read(NIFStream* nif)
    {
        mHavokMaterial.read(nif);
    }

    void bhkConvexShape::read(NIFStream* nif)
    {
        bhkSphereRepShape::read(nif);
        mRadius = nif->getFloat();
    }

    void bhkConvexVerticesShape::read(NIFStream* nif)
    {
        bhkConvexShape::read(nif);
        mVerticesProperty.read(nif);
        mNormalsProperty.read(nif);
        unsigned int numVertices = nif->getUInt();
        if (numVertices)
            nif->getVector4s(mVertices, numVertices);
        unsigned int numNormals = nif->getUInt();
        if (numNormals)
            nif->getVector4s(mNormals, numNormals);
    }

    void bhkConvexTransformShape::read(NIFStream* nif)
    {
        mShape.read(nif);
        mHavokMaterial.read(nif);
        mRadius = nif->getFloat();
        nif->skip(8); // Unused
        std::array<float, 16> mat;
        nif->readArray(mat);
        mTransform.set(mat.data());
    }

    void bhkConvexTransformShape::post(Reader& nif)
    {
        mShape.post(nif);
    }

    void bhkBoxShape::read(NIFStream* nif)
    {
        bhkConvexShape::read(nif);
        nif->skip(8); // Unused
        mExtents = nif->getVector3();
        nif->skip(4); // Unused
    }

    void bhkCapsuleShape::read(NIFStream* nif)
    {
        bhkConvexShape::read(nif);
        nif->skip(8); // Unused
        mPoint1 = nif->getVector3();
        mRadius1 = nif->getFloat();
        mPoint2 = nif->getVector3();
        mRadius2 = nif->getFloat();
    }

    void bhkListShape::read(NIFStream* nif)
    {
        readRecordList(nif, mSubshapes);
        mHavokMaterial.read(nif);
        mChildShapeProperty.read(nif);
        mChildFilterProperty.read(nif);
        unsigned int numFilters = nif->getUInt();
        mHavokFilters.resize(numFilters);
        for (HavokFilter& filter : mHavokFilters)
            filter.read(nif);
    }

    void bhkCompressedMeshShape::read(NIFStream* nif)
    {
        mTarget.read(nif);
        mUserData = nif->getUInt();
        mRadius = nif->getFloat();
        nif->getFloat(); // Unknown
        mScale = nif->getVector4();
        nif->getFloat(); // Radius
        nif->getVector4(); // Scale
        mData.read(nif);
    }

    void bhkCompressedMeshShape::post(Reader& nif)
    {
        mTarget.post(nif);
        mData.post(nif);
    }

    void bhkCompressedMeshShapeData::read(NIFStream* nif)
    {
        mBitsPerIndex = nif->getUInt();
        mBitsPerWIndex = nif->getUInt();
        mMaskWIndex = nif->getUInt();
        mMaskIndex = nif->getUInt();
        mError = nif->getFloat();
        mAabbMin = nif->getVector4();
        mAabbMax = nif->getVector4();
        mWeldingType = nif->getChar();
        mMaterialType = nif->getChar();
        nif->skip(nif->getUInt() * 4); // Unused
        nif->skip(nif->getUInt() * 4); // Unused
        nif->skip(nif->getUInt() * 4); // Unused

        size_t numMaterials = nif->getUInt();
        mMaterials.resize(numMaterials);
        for (bhkMeshMaterial& material : mMaterials)
            material.read(nif);

        nif->getUInt(); // Unused
        size_t numTransforms = nif->getUInt();

        mChunkTransforms.resize(numTransforms);
        for (bhkQsTransform& transform : mChunkTransforms)
            transform.read(nif);

        size_t numBigVertices = nif->getUInt();
        if (numBigVertices)
            nif->getVector4s(mBigVerts, numBigVertices);

        size_t numBigTriangles = nif->getUInt();
        mBigTris.resize(numBigTriangles);
        for (bhkCMSBigTri& tri : mBigTris)
            tri.read(nif);

        size_t numChunks = nif->getUInt();
        mChunks.resize(numChunks);
        for (bhkCMSChunk& chunk : mChunks)
            chunk.read(nif);

        nif->getUInt(); // Unused
    }

    void bhkRigidBody::read(NIFStream* nif)
    {
        bhkEntity::read(nif);
        mInfo.read(nif);
        readRecordList(nif, mConstraints);
        if (nif->getBethVersion() < 76)
            mBodyFlags = nif->getUInt();
        else
            mBodyFlags = nif->getUShort();
    }

    void bhkSimpleShapePhantom::read(NIFStream* nif)
    {
        bhkWorldObject::read(nif);
        nif->skip(8); // Unused
        std::array<float, 16> mat;
        nif->readArray(mat);
        mTransform.set(mat.data());
    }

    void bhkConstraint::read(NIFStream* nif)
    {
        mInfo.read(nif);
    }

    void bhkConstraint::post(Reader& nif)
    {
        mInfo.post(nif);
    }

} // Namespace
