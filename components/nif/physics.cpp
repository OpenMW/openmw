#include "physics.hpp"

#include "data.hpp"
#include "node.hpp"

#include <array>

namespace Nif
{

    /// Non-record data types

    void bhkWorldObjCInfoProperty::read(NIFStream* nif)
    {
        nif->read(mData);
        nif->read(mSize);
        nif->read(mCapacityAndFlags);
    }

    void bhkWorldObjectCInfo::read(NIFStream* nif)
    {
        nif->skip(4); // Unused
        mPhaseType = static_cast<BroadPhaseType>(nif->get<uint8_t>());
        nif->skip(3); // Unused
        mProperty.read(nif);
    }

    void HavokMaterial::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->skip(4); // Unknown
        nif->read(mMaterial);
    }

    void HavokFilter::read(NIFStream* nif)
    {
        nif->read(mLayer);
        nif->read(mFlags);
        nif->read(mGroup);
    }

    void hkSubPartData::read(NIFStream* nif)
    {
        mHavokFilter.read(nif);
        nif->read(mNumVertices);
        mHavokMaterial.read(nif);
    }

    void bhkEntityCInfo::read(NIFStream* nif)
    {
        mResponseType = static_cast<HkResponseType>(nif->get<uint8_t>());
        nif->skip(1); // Unused
        nif->read(mProcessContactDelay);
    }

    void hkpMoppCode::read(NIFStream* nif)
    {
        uint32_t dataSize;
        nif->read(dataSize);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mOffset);
        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
            nif->read(mBuildType);
        nif->readVector(mData, dataSize);
    }

    void TriangleData::read(NIFStream* nif)
    {
        nif->readArray(mTriangle);
        nif->read(mWeldingInfo);
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
            nif->read(mNormal);
    }

    void bhkMeshMaterial::read(NIFStream* nif)
    {
        mHavokMaterial.read(nif);
        mHavokFilter.read(nif);
    }

    void bhkQsTransform::read(NIFStream* nif)
    {
        nif->read(mTranslation);
        nif->read(mRotation);
    }

    void bhkCMSBigTri::read(NIFStream* nif)
    {
        nif->readArray(mTriangle);
        nif->read(mMaterial);
        nif->read(mWeldingInfo);
    }

    void bhkCMSChunk::read(NIFStream* nif)
    {
        nif->read(mTranslation);
        nif->read(mMaterialIndex);
        nif->read(mReference);
        nif->read(mTransformIndex);
        nif->readVector(mVertices, nif->get<uint32_t>());
        nif->readVector(mIndices, nif->get<uint32_t>());
        nif->readVector(mStrips, nif->get<uint32_t>());
        nif->readVector(mWeldingInfos, nif->get<uint32_t>());
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
                mResponseType = static_cast<HkResponseType>(nif->get<uint8_t>());
                nif->skip(1); // Unused
                nif->read(mProcessContactDelay);
            }
        }
        if (nif->getBethVersion() < 83)
            nif->skip(4); // Unused
        nif->read(mTranslation);
        nif->read(mRotation);
        nif->read(mLinearVelocity);
        nif->read(mAngularVelocity);
        // A bit hacky, but this is the only instance where a 3x3 matrix has padding.
        for (int i = 0; i < 3; i++)
        {
            nif->read(mInertiaTensor.mValues[i], 3);
            nif->skip(4); // Padding
        }
        nif->read(mCenter);
        nif->read(mMass);
        nif->read(mLinearDamping);
        nif->read(mAngularDamping);
        if (nif->getBethVersion() >= 83)
        {
            if (nif->getBethVersion() != NIFFile::BethVersion::BETHVER_FO4)
                nif->read(mTimeFactor);
            nif->read(mGravityFactor);
        }
        nif->read(mFriction);
        if (nif->getBethVersion() >= 83)
            nif->read(mRollingFrictionMult);
        nif->read(mRestitution);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            nif->read(mMaxLinearVelocity);
            nif->read(mMaxAngularVelocity);
            if (nif->getBethVersion() != NIFFile::BethVersion::BETHVER_FO4)
                nif->read(mPenetrationDepth);
        }
        mMotionType = static_cast<HkMotionType>(nif->get<uint8_t>());
        if (nif->getBethVersion() < 83)
            mDeactivatorType = static_cast<HkDeactivatorType>(nif->get<uint8_t>());
        else
            nif->read(mEnableDeactivation);
        mSolverDeactivation = static_cast<HkSolverDeactivation>(nif->get<uint8_t>());
        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_FO4)
        {
            nif->skip(1);
            nif->read(mPenetrationDepth);
            nif->read(mTimeFactor);
            nif->skip(4);
            mResponseType = static_cast<HkResponseType>(nif->get<uint8_t>());
            nif->skip(1); // Unused
            nif->read(mProcessContactDelay);
        }
        mQualityType = static_cast<HkQualityType>(nif->get<uint8_t>());
        if (nif->getBethVersion() >= 83)
        {
            nif->read(mAutoRemoveLevel);
            nif->read(mResponseModifierFlags);
            nif->read(mNumContactPointShapeKeys);
            nif->read(mForceCollidedOntoPPU);
        }
        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_FO4)
            nif->skip(3); // Unused
        else
            nif->skip(12); // Unused
    }

    void bhkConstraintCInfo::read(NIFStream* nif)
    {
        nif->get<uint32_t>(); // Number of entities, unused
        mEntityA.read(nif);
        mEntityB.read(nif);

        mPriority = static_cast<ConstraintPriority>(nif->get<uint32_t>());
    }

    void bhkConstraintCInfo::post(Reader& nif)
    {
        mEntityA.post(nif);
        mEntityB.post(nif);
    }

    void bhkPositionConstraintMotor::read(NIFStream* nif)
    {
        nif->read(mMinForce);
        nif->read(mMaxForce);
        nif->read(mTau);
        nif->read(mDamping);
        nif->read(mProportionalRecoveryVelocity);
        nif->read(mConstantRecoveryVelocity);
        nif->read(mEnabled);
    }

    void bhkVelocityConstraintMotor::read(NIFStream* nif)
    {
        nif->read(mMinForce);
        nif->read(mMaxForce);
        nif->read(mTau);
        nif->read(mTargetVelocity);
        nif->read(mUseVelocityTarget);
        nif->read(mEnabled);
    }

    void bhkSpringDamperConstraintMotor::read(NIFStream* nif)
    {
        nif->read(mMinForce);
        nif->read(mMaxForce);
        nif->read(mSpringConstant);
        nif->read(mSpringDamping);
        nif->read(mEnabled);
    }

    void bhkConstraintMotorCInfo::read(NIFStream* nif)
    {
        mType = static_cast<HkMotorType>(nif->get<uint8_t>());
        switch (mType)
        {
            case HkMotorType::Motor_Position:
                mPositionMotor.read(nif);
                break;
            case HkMotorType::Motor_Velocity:
                mVelocityMotor.read(nif);
                break;
            case HkMotorType::Motor_SpringDamper:
                mSpringDamperMotor.read(nif);
                break;
            case HkMotorType::Motor_None:
            default:
                break;
        }
    }

    void bhkRagdollConstraintCInfo::read(NIFStream* nif)
    {
        if (nif->getBethVersion() <= 16)
        {
            nif->read(mDataA.mPivot);
            nif->read(mDataA.mPlane);
            nif->read(mDataA.mTwist);
            nif->read(mDataB.mPivot);
            nif->read(mDataB.mPlane);
            nif->read(mDataB.mTwist);
        }
        else
        {
            nif->read(mDataA.mTwist);
            nif->read(mDataA.mPlane);
            nif->read(mDataA.mMotor);
            nif->read(mDataA.mPivot);
            nif->read(mDataB.mTwist);
            nif->read(mDataB.mPlane);
            nif->read(mDataB.mMotor);
            nif->read(mDataB.mPivot);
        }
        nif->read(mConeMaxAngle);
        nif->read(mPlaneMinAngle);
        nif->read(mPlaneMaxAngle);
        nif->read(mTwistMinAngle);
        nif->read(mTwistMaxAngle);
        nif->read(mMaxFriction);
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() > 16)
            mMotor.read(nif);
    }

    void bhkHingeConstraintCInfo::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            nif->read(mDataA.mPivot);
            nif->read(mDataA.mPerpAxis1);
            nif->read(mDataA.mPerpAxis2);
            nif->read(mDataB.mPivot);
            nif->read(mDataB.mAxis);
        }
        else
        {
            nif->read(mDataA.mAxis);
            nif->read(mDataA.mPerpAxis1);
            nif->read(mDataA.mPerpAxis2);
            nif->read(mDataA.mPivot);
            nif->read(mDataB.mAxis);
            nif->read(mDataB.mPerpAxis1);
            nif->read(mDataB.mPerpAxis2);
            nif->read(mDataB.mPivot);
        }
    }

    void bhkLimitedHingeConstraintCInfo::read(NIFStream* nif)
    {
        if (nif->getBethVersion() <= 16)
        {
            nif->read(mDataA.mPivot);
            nif->read(mDataA.mAxis);
            nif->read(mDataA.mPerpAxis1);
            nif->read(mDataA.mPerpAxis2);
            nif->read(mDataB.mPivot);
            nif->read(mDataB.mAxis);
            nif->read(mDataB.mPerpAxis2);
        }
        else
        {
            nif->read(mDataA.mAxis);
            nif->read(mDataA.mPerpAxis1);
            nif->read(mDataA.mPerpAxis2);
            nif->read(mDataA.mPivot);
            nif->read(mDataB.mAxis);
            nif->read(mDataB.mPerpAxis1);
            nif->read(mDataB.mPerpAxis2);
            nif->read(mDataB.mPivot);
        }
        nif->read(mMinAngle);
        nif->read(mMaxAngle);
        nif->read(mMaxFriction);
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() > 16)
            mMotor.read(nif);
    }

    void bhkBallAndSocketConstraintCInfo::read(NIFStream* nif)
    {
        nif->read(mPivotA);
        nif->read(mPivotB);
    }

    void bhkStiffSpringConstraintCInfo::read(NIFStream* nif)
    {
        nif->read(mPivotA);
        nif->read(mPivotB);
        nif->read(mLength);
    }

    void bhkPrismaticConstraintCInfo::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            nif->read(mDataA.mPivot);
            nif->read(mDataA.mRotation);
            nif->read(mDataA.mPlane);
            nif->read(mDataA.mSliding);
            nif->read(mDataB.mSliding);
            nif->read(mDataB.mPivot);
            nif->read(mDataB.mRotation);
            nif->read(mDataB.mPlane);
        }
        else
        {
            nif->read(mDataA.mSliding);
            nif->read(mDataA.mRotation);
            nif->read(mDataA.mPlane);
            nif->read(mDataA.mPivot);
            nif->read(mDataB.mSliding);
            nif->read(mDataB.mRotation);
            nif->read(mDataB.mPlane);
            nif->read(mDataB.mPivot);
        }
        nif->read(mMinDistance);
        nif->read(mMaxDistance);
        nif->read(mFriction);
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() >= 17)
            mMotor.read(nif);
    }

    void bhkMalleableConstraintCInfo::read(NIFStream* nif)
    {
        mType = static_cast<HkConstraintType>(nif->get<uint32_t>());
        mInfo.read(nif);
        switch (mType)
        {
            case HkConstraintType::BallAndSocket:
                mBallAndSocketInfo.read(nif);
                break;
            case HkConstraintType::Hinge:
                mHingeInfo.read(nif);
                break;
            case HkConstraintType::LimitedHinge:
                mLimitedHingeInfo.read(nif);
                break;
            case HkConstraintType::Prismatic:
                mPrismaticInfo.read(nif);
                break;
            case HkConstraintType::Ragdoll:
                mRagdollInfo.read(nif);
                break;
            case HkConstraintType::StiffSpring:
                mStiffSpringInfo.read(nif);
                break;
            default:
                throw Nif::Exception(
                    "Unrecognized constraint type in bhkMalleableConstraint", nif->getFile().getFilename());
        }
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            nif->read(mTau);
            nif->read(mDamping);
        }
        else
        {
            nif->read(mStrength);
        }
    }

    void bhkWrappedConstraintData::read(NIFStream* nif)
    {
        mType = static_cast<HkConstraintType>(nif->get<uint32_t>());
        mInfo.read(nif);
        switch (mType)
        {
            case HkConstraintType::BallAndSocket:
                mBallAndSocketInfo.read(nif);
                break;
            case HkConstraintType::Hinge:
                mHingeInfo.read(nif);
                break;
            case HkConstraintType::LimitedHinge:
                mLimitedHingeInfo.read(nif);
                break;
            case HkConstraintType::Prismatic:
                mPrismaticInfo.read(nif);
                break;
            case HkConstraintType::Ragdoll:
                mRagdollInfo.read(nif);
                break;
            case HkConstraintType::StiffSpring:
                mStiffSpringInfo.read(nif);
                break;
            case HkConstraintType::Malleable:
                mMalleableInfo.read(nif);
                break;
            default:
                throw Nif::Exception(
                    "Unrecognized constraint type in bhkWrappedConstraintData", nif->getFile().getFilename());
        }
    }

    void bhkConstraintChainCInfo::read(NIFStream* nif)
    {
        readRecordList(nif, mEntities);
        mInfo.read(nif);
    }

    void bhkConstraintChainCInfo::post(Reader& nif)
    {
        postRecordList(nif, mEntities);
    }

    /// Record types

    void bhkCollisionObject::read(NIFStream* nif)
    {
        NiCollisionObject::read(nif);

        nif->read(mFlags);
        mBody.read(nif);
    }

    void bhkNPCollisionObject::read(NIFStream* nif)
    {
        NiCollisionObject::read(nif);

        nif->read(mFlags);
        mData.read(nif);
        nif->read(mBodyID);
    }

    void bhkNPCollisionObject::post(Reader& nif)
    {
        NiCollisionObject::post(nif);

        mData.post(nif);
    }

    void bhkBlendCollisionObject::read(NIFStream* nif)
    {
        bhkCollisionObject::read(nif);

        nif->read(mHeirGain);
        nif->read(mVelGain);

        if (nif->getBethVersion() <= 8)
            nif->skip(8); // Unknown
    }

    void bhkPhysicsSystem::read(NIFStream* nif)
    {
        nif->readVector(mData, nif->get<uint32_t>());
    }

    void bhkRagdollSystem::read(NIFStream* nif)
    {
        nif->readVector(mData, nif->get<uint32_t>());
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
        nif->read(mScale);
        mMopp.read(nif);
    }

    void bhkNiTriStripsShape::read(NIFStream* nif)
    {
        mHavokMaterial.read(nif);
        nif->read(mRadius);
        nif->skip(20); // Unused
        nif->read(mGrowBy);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mScale);
        readRecordList(nif, mData);
        nif->readVectorOfRecords<uint32_t>(mHavokFilters);
    }

    void bhkNiTriStripsShape::post(Reader& nif)
    {
        postRecordList(nif, mData);
    }

    void bhkPackedNiTriStripsShape::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            uint16_t numSubshapes;
            nif->read(numSubshapes);
            mSubshapes.resize(numSubshapes);
            for (hkSubPartData& subshape : mSubshapes)
                subshape.read(nif);
        }
        nif->read(mUserData);
        nif->skip(4); // Unused
        nif->read(mRadius);
        nif->skip(4); // Unused
        nif->read(mScale);
        nif->skip(20); // Duplicates of the two previous fields
        mData.read(nif);
    }

    void bhkPackedNiTriStripsShape::post(Reader& nif)
    {
        mData.post(nif);
    }

    void hkPackedNiTriStripsData::read(NIFStream* nif)
    {
        uint32_t numTriangles;
        nif->read(numTriangles);
        mTriangles.resize(numTriangles);
        for (uint32_t i = 0; i < numTriangles; i++)
            mTriangles[i].read(nif);

        uint32_t numVertices;
        nif->read(numVertices);
        bool compressed = false;
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS)
            nif->read(compressed);
        if (!compressed)
            nif->readVector(mVertices, numVertices);
        else
            nif->skip(6 * numVertices); // Half-precision vectors are not currently supported
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS)
        {
            uint16_t numSubshapes;
            nif->read(numSubshapes);
            mSubshapes.resize(numSubshapes);
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

        nif->read(mRadius);
    }

    void bhkConvexListShape::read(NIFStream* nif)
    {
        readRecordList(nif, mSubShapes);
        mMaterial.read(nif);
        nif->read(mRadius);
        nif->skip(8); // Unknown
        mChildShapeProperty.read(nif);
        nif->read(mUseCachedAABB);
        nif->read(mClosestPointMinDistance);
    }

    void bhkConvexListShape::post(Reader& nif)
    {
        postRecordList(nif, mSubShapes);
    }

    void bhkConvexSweepShape::read(NIFStream* nif)
    {
        mShape.read(nif);
        mMaterial.read(nif);
        nif->read(mRadius);
        nif->skip(12); // Unknown
    }

    void bhkConvexSweepShape::post(Reader& nif)
    {
        mShape.post(nif);
    }

    void bhkConvexVerticesShape::read(NIFStream* nif)
    {
        bhkConvexShape::read(nif);

        mVerticesProperty.read(nif);
        mNormalsProperty.read(nif);
        nif->readVector(mVertices, nif->get<uint32_t>());
        nif->readVector(mNormals, nif->get<uint32_t>());
    }

    void bhkConvexTransformShape::read(NIFStream* nif)
    {
        mShape.read(nif);
        mHavokMaterial.read(nif);
        nif->read(mRadius);
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
        nif->read(mExtents);
        nif->skip(4); // Unused
    }

    void bhkCapsuleShape::read(NIFStream* nif)
    {
        bhkConvexShape::read(nif);

        nif->skip(8); // Unused
        nif->read(mPoint1);
        nif->read(mRadius1);
        nif->read(mPoint2);
        nif->read(mRadius2);
    }

    void bhkCylinderShape::read(NIFStream* nif)
    {
        bhkConvexShape::read(nif);

        nif->skip(8); // Unused
        nif->read(mVertexA);
        nif->read(mVertexB);
        nif->read(mCylinderRadius);
        nif->skip(12); // Unused
    }

    void bhkHeightfieldShape::read(NIFStream* nif)
    {
        mHavokMaterial.read(nif);
    }

    void bhkPlaneShape::read(NIFStream* nif)
    {
        bhkHeightfieldShape::read(nif);

        nif->skip(12); // Unused
        mPlane = osg::Plane(nif->get<osg::Vec4f>());
        nif->read(mExtents);
        nif->read(mCenter);
    }

    void bhkMeshShape::read(NIFStream* nif)
    {
        nif->skip(8); // Unknown
        nif->read(mRadius);
        nif->skip(8); // Unknown
        nif->read(mScale);
        mShapeProperties.resize(nif->get<uint32_t>());
        for (bhkWorldObjCInfoProperty& property : mShapeProperties)
            property.read(nif);
        nif->skip(12); // Unknown
        readRecordList(nif, mDataList);
    }

    void bhkMeshShape::post(Reader& nif)
    {
        postRecordList(nif, mDataList);
    }

    void bhkMultiSphereShape::read(NIFStream* nif)
    {
        bhkSphereRepShape::read(nif);

        mShapeProperty.read(nif);
        nif->readVector(mSpheres, nif->get<uint32_t>());
    }

    void bhkListShape::read(NIFStream* nif)
    {
        readRecordList(nif, mSubshapes);
        mHavokMaterial.read(nif);
        mChildShapeProperty.read(nif);
        mChildFilterProperty.read(nif);
        uint32_t numFilters;
        nif->read(numFilters);
        mHavokFilters.resize(numFilters);
        for (HavokFilter& filter : mHavokFilters)
            filter.read(nif);
    }

    void bhkListShape::post(Reader& nif)
    {
        postRecordList(nif, mSubshapes);
    }

    void bhkCompressedMeshShape::read(NIFStream* nif)
    {
        mTarget.read(nif);
        nif->read(mUserData);
        nif->read(mRadius);
        nif->skip(4); // Unknown
        nif->read(mScale);
        nif->skip(4); // Radius
        nif->skip(16); // Scale
        mData.read(nif);
    }

    void bhkCompressedMeshShape::post(Reader& nif)
    {
        mTarget.post(nif);
        mData.post(nif);
    }

    void bhkCompressedMeshShapeData::read(NIFStream* nif)
    {
        nif->read(mBitsPerIndex);
        nif->read(mBitsPerWIndex);
        nif->read(mMaskWIndex);
        nif->read(mMaskIndex);
        nif->read(mError);
        nif->read(mAabbMin);
        nif->read(mAabbMax);
        nif->read(mWeldingType);
        nif->read(mMaterialType);
        nif->skip(nif->get<uint32_t>() * 4); // Unused
        nif->skip(nif->get<uint32_t>() * 4); // Unused
        nif->skip(nif->get<uint32_t>() * 4); // Unused

        uint32_t numMaterials;
        nif->read(numMaterials);
        mMaterials.resize(numMaterials);
        for (bhkMeshMaterial& material : mMaterials)
            material.read(nif);

        nif->skip(4); // Unused

        uint32_t numTransforms;
        nif->read(numTransforms);
        mChunkTransforms.resize(numTransforms);
        for (bhkQsTransform& transform : mChunkTransforms)
            transform.read(nif);

        nif->readVector(mBigVerts, nif->get<uint32_t>());

        uint32_t numBigTriangles;
        nif->read(numBigTriangles);
        mBigTris.resize(numBigTriangles);
        for (bhkCMSBigTri& tri : mBigTris)
            tri.read(nif);

        uint32_t numChunks;
        nif->read(numChunks);
        mChunks.resize(numChunks);
        for (bhkCMSChunk& chunk : mChunks)
            chunk.read(nif);

        nif->skip(4); // Unused
    }

    void bhkRigidBody::read(NIFStream* nif)
    {
        bhkEntity::read(nif);

        mInfo.read(nif);
        readRecordList(nif, mConstraints);
        if (nif->getBethVersion() < 76)
            nif->read(mBodyFlags);
        else
            mBodyFlags = nif->get<uint16_t>();
    }

    void bhkAabbPhantom::read(NIFStream* nif)
    {
        bhkPhantom::read(nif);

        nif->skip(8); // Unused
        nif->read(mAabbMin);
        nif->read(mAabbMax);
    }

    void bhkSimpleShapePhantom::read(NIFStream* nif)
    {
        bhkShapePhantom::read(nif);

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

    void bhkRagdollConstraint::read(NIFStream* nif)
    {
        bhkConstraint::read(nif);

        mConstraint.read(nif);
    }

    void bhkHingeConstraint::read(NIFStream* nif)
    {
        bhkConstraint::read(nif);

        mConstraint.read(nif);
    }

    void bhkLimitedHingeConstraint::read(NIFStream* nif)
    {
        bhkConstraint::read(nif);

        mConstraint.read(nif);
    }

    void bhkBallAndSocketConstraint::read(NIFStream* nif)
    {
        bhkConstraint::read(nif);

        mConstraint.read(nif);
    }

    void bhkBallSocketConstraintChain::read(NIFStream* nif)
    {
        uint32_t numPivots = nif->get<uint32_t>();
        if (numPivots % 2 != 0)
            throw Nif::Exception(
                "Invalid number of constraints in bhkBallSocketConstraintChain", nif->getFile().getFilename());
        mConstraints.resize(numPivots / 2);
        for (bhkBallAndSocketConstraintCInfo& info : mConstraints)
            info.read(nif);
        nif->read(mTau);
        nif->read(mDamping);
        nif->read(mConstraintForceMixing);
        nif->read(mMaxErrorDistance);
        mConstraintChainInfo.read(nif);
    }

    void bhkBallSocketConstraintChain::post(Reader& nif)
    {
        mConstraintChainInfo.post(nif);
    }

    void bhkStiffSpringConstraint::read(NIFStream* nif)
    {
        bhkConstraint::read(nif);

        mConstraint.read(nif);
    }

    void bhkPrismaticConstraint::read(NIFStream* nif)
    {
        bhkConstraint::read(nif);

        mConstraint.read(nif);
    }

    void bhkMalleableConstraint::read(NIFStream* nif)
    {
        bhkConstraint::read(nif);

        mConstraint.read(nif);
    }

    void bhkBreakableConstraint::read(NIFStream* nif)
    {
        bhkConstraint::read(nif);

        mConstraint.read(nif);
        nif->read(mThreshold);
        nif->read(mRemoveWhenBroken);
    }

    void bhkUnaryAction::read(NIFStream* nif)
    {
        mEntity.read(nif);
        nif->skip(8); // Unused
    }

    void bhkUnaryAction::post(Reader& nif)
    {
        mEntity.post(nif);
    }

    void bhkLiquidAction::read(NIFStream* nif)
    {
        nif->skip(12); // Unused
        nif->read(mInitialStickForce);
        nif->read(mStickStrength);
        nif->read(mNeighborDistance);
        nif->read(mNeighborStrength);
    }

    void bhkOrientHingedBodyAction::read(NIFStream* nif)
    {
        bhkUnaryAction::read(nif);

        nif->skip(8); // Unused
        nif->read(mHingeAxisLS);
        nif->read(mForwardLS);
        nif->read(mStrength);
        nif->read(mDamping);
        nif->skip(8); // Unused
    }

    void bhkRagdollTemplate::read(NIFStream* nif)
    {
        Extra::read(nif);

        readRecordList(nif, mBones);
    }

    void bhkRagdollTemplate::post(Reader& nif)
    {
        Extra::post(nif);

        postRecordList(nif, mBones);
    }

    void bhkRagdollTemplateData::read(NIFStream* nif)
    {
        nif->read(mName);
        nif->read(mMass);
        nif->read(mRestitution);
        nif->read(mFriction);
        nif->read(mRadius);
        mHavokMaterial.read(nif);
        mConstraints.resize(nif->get<uint32_t>());
        for (bhkWrappedConstraintData& constraint : mConstraints)
            constraint.read(nif);
    }

    void bhkPoseArray::BoneTransform::read(NIFStream* nif)
    {
        nif->read(mTranslation);
        nif->read(mRotation);
        nif->read(mScale);
    }

    void bhkPoseArray::read(NIFStream* nif)
    {
        nif->readVector(mBones, nif->get<uint32_t>());
        mPoses.resize(nif->get<uint32_t>());
        for (std::vector<BoneTransform>& pose : mPoses)
        {
            pose.resize(nif->get<uint32_t>());
            for (BoneTransform& transform : pose)
                transform.read(nif);
        }
    }

} // Namespace
