#include "physics.hpp"

#include "data.hpp"
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

    void hkpMoppCode::read(NIFStream *nif)
    {
        unsigned int size = nif->getUInt();
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
            mOffset = nif->getVector4();
        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
            nif->getChar(); // MOPP data build type
        if (size)
            nif->getChars(mData, size);
    }

    void bhkEntityCInfo::read(NIFStream *nif)
    {
        mResponseType = static_cast<hkResponseType>(nif->getChar());
        nif->skip(1); // Unused
        mProcessContactDelay = nif->getUShort();
    }

    void TriangleData::read(NIFStream *nif)
    {
        for (int i = 0; i < 3; i++)
            mTriangle[i] = nif->getUShort();
        mWeldingInfo = nif->getUShort();
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
            mNormal = nif->getVector3();
    }

    void bhkRigidBodyCInfo::read(NIFStream *nif)
    {
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
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
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
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

    void bhkBvTreeShape::read(NIFStream *nif)
    {
        mShape.read(nif);
    }

    void bhkBvTreeShape::post(NIFFile *nif)
    {
        mShape.post(nif);
    }

    void bhkMoppBvTreeShape::read(NIFStream *nif)
    {
        bhkBvTreeShape::read(nif);
        nif->skip(12); // Unused
        mScale = nif->getFloat();
        mMopp.read(nif);
    }

    void bhkNiTriStripsShape::read(NIFStream *nif)
    {
        mHavokMaterial.read(nif);
        mRadius = nif->getFloat();
        nif->skip(20); // Unused
        mGrowBy = nif->getUInt();
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
            mScale = nif->getVector4();
        mData.read(nif);
        unsigned int numFilters = nif->getUInt();
        nif->getUInts(mFilters, numFilters);
    }

    void bhkNiTriStripsShape::post(NIFFile *nif)
    {
        mData.post(nif);
    }

    void bhkPackedNiTriStripsShape::read(NIFStream *nif)
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

    void bhkPackedNiTriStripsShape::post(NIFFile *nif)
    {
        mData.post(nif);
    }

    void hkPackedNiTriStripsData::read(NIFStream *nif)
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

    void bhkSphereRepShape::read(NIFStream *nif)
    {
        mHavokMaterial.read(nif);
    }

    void bhkConvexShape::read(NIFStream *nif)
    {
        bhkSphereRepShape::read(nif);
        mRadius = nif->getFloat();
    }

    void bhkConvexVerticesShape::read(NIFStream *nif)
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

    void bhkBoxShape::read(NIFStream *nif)
    {
        bhkConvexShape::read(nif);
        nif->skip(8); // Unused
        mExtents = nif->getVector3();
        nif->skip(4); // Unused
    }

    void bhkListShape::read(NIFStream *nif)
    {
        mSubshapes.read(nif);
        mHavokMaterial.read(nif);
        mChildShapeProperty.read(nif);
        mChildFilterProperty.read(nif);
        unsigned int numFilters = nif->getUInt();
        mHavokFilters.resize(numFilters);
        for (HavokFilter& filter : mHavokFilters)
            filter.read(nif);
    }

    void bhkRigidBody::read(NIFStream *nif)
    {
        bhkEntity::read(nif);
        mInfo.read(nif);
        mConstraints.read(nif);
        if (nif->getBethVersion() < 76)
            mBodyFlags = nif->getUInt();
        else
            mBodyFlags = nif->getUShort();
    }

} // Namespace
