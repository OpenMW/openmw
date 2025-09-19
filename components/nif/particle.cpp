#include "particle.hpp"

#include "data.hpp"

namespace Nif
{

    void NiParticleModifier::read(NIFStream* nif)
    {
        mNext.read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            mController.read(nif);
    }

    void NiParticleModifier::post(Reader& nif)
    {
        mNext.post(nif);
        mController.post(nif);
    }

    void NiParticleGrowFade::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);
        nif->read(mGrowTime);
        nif->read(mFadeTime);
    }

    void NiParticleColorModifier::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        mData.read(nif);
    }

    void NiParticleColorModifier::post(Reader& nif)
    {
        NiParticleModifier::post(nif);

        mData.post(nif);
    }

    void NiGravity::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            nif->read(mDecay);
        nif->read(mForce);
        mType = static_cast<ForceType>(nif->get<uint32_t>());
        nif->read(mPosition);
        nif->read(mDirection);
    }

    void NiParticleBomb::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        nif->read(mRange);
        nif->read(mDuration);
        nif->read(mStrength);
        nif->read(mStartTime);
        mDecayType = static_cast<DecayType>(nif->get<uint32_t>());
        mSymmetryType = static_cast<SymmetryType>(nif->get<uint32_t>());
        nif->read(mPosition);
        nif->read(mDirection);
    }

    void NiParticleCollider::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        nif->read(mBounceFactor);
        if (nif->getVersion() >= NIFStream::generateVersion(4, 2, 0, 2))
        {
            nif->read(mSpawnOnCollision);
            nif->read(mDieOnCollision);
        }
    }

    void NiPlanarCollider::read(NIFStream* nif)
    {
        NiParticleCollider::read(nif);

        nif->read(mExtents);
        nif->read(mPosition);
        nif->read(mXVector);
        nif->read(mYVector);
        nif->read(mPlaneNormal);
        nif->read(mPlaneDistance);
    }

    void NiSphericalCollider::read(NIFStream* nif)
    {
        NiParticleCollider::read(nif);

        nif->read(mRadius);
        nif->read(mCenter);
    }

    void NiParticleRotation::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        nif->read(mRandomInitialAxis);
        nif->read(mInitialAxis);
        nif->read(mRotationSpeed);
    }

    void NiParticlesData::read(NIFStream* nif)
    {
        NiGeometryData::read(nif);

        // Should always match the number of vertices in theory, but doesn't:
        // see mist.nif in Mistify mod (https://www.nexusmods.com/morrowind/mods/48112).
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_MW)
            nif->read(mNumParticles);
        bool isBs202 = nif->getVersion() == NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() != 0;

        uint16_t numRadii = 1;
        if (nif->getVersion() > NIFStream::generateVersion(10, 0, 1, 0))
            numRadii = (nif->get<bool>() && !isBs202) ? mNumVertices : 0;
        nif->readVector(mRadii, numRadii);
        nif->read(mActiveCount);
        if (nif->get<bool>() && !isBs202)
            nif->readVector(mSizes, mNumVertices);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
        {
            if (nif->get<bool>() && !isBs202)
                nif->readVector(mRotations, mNumVertices);
            if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 4))
            {
                if (nif->get<bool>() && !isBs202)
                    nif->readVector(mRotationAngles, mNumVertices);
                if (nif->get<bool>() && !isBs202)
                    nif->readVector(mRotationAxes, mNumVertices);
                if (isBs202)
                {
                    nif->read(mHasTextureIndices);
                    uint32_t numSubtextureOffsets;
                    if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
                        numSubtextureOffsets = nif->get<uint8_t>();
                    else
                        nif->read(numSubtextureOffsets);
                    nif->readVector(mSubtextureOffsets, numSubtextureOffsets);
                    if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
                    {
                        nif->read(mAspectRatio);
                        nif->read(mAspectFlags);
                        nif->read(mAspectRatio2);
                        nif->read(mAspectSpeed);
                        nif->read(mAspectSpeed2);
                    }
                }
            }
        }
    }

    void NiRotatingParticlesData::read(NIFStream* nif)
    {
        NiParticlesData::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(4, 2, 2, 0) && nif->get<bool>())
            nif->readVector(mRotations, mNumVertices);
    }

    void BSMasterParticleSystem::read(NIFStream* nif)
    {
        NiNode::read(nif);

        nif->read(mMaxEmitters);
        readRecordList(nif, mParticleSystems);
    }

    void BSMasterParticleSystem::post(Reader& nif)
    {
        NiNode::post(nif);

        postRecordList(nif, mParticleSystems);
    }

    void NiParticleSystem::read(NIFStream* nif)
    {
        // Weird loading to account for inheritance differences starting from SSE
        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_SSE)
            NiParticles::read(nif);
        else
        {
            NiAVObject::read(nif);

            nif->read(mBoundingSphere);
            if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
                nif->readArray(mBoundMinMax);

            mSkin.read(nif);
            mShaderProperty.read(nif);
            mAlphaProperty.read(nif);
            mVertDesc.read(nif);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_SKY)
        {
            nif->readArray(mNearFar);
            if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_SSE)
                mData.read(nif);
        }

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            nif->read(mWorldSpace);
            readRecordList(nif, mModifiers);
        }
    }

    void NiParticleSystem::post(Reader& nif)
    {
        NiParticles::post(nif);

        postRecordList(nif, mModifiers);
    }

    void NiPSysData::read(NIFStream* nif)
    {
        NiParticlesData::read(nif);

        bool hasData = nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO3;
        if (hasData)
            nif->readVectorOfRecords(mNumVertices, mParticles);

        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO4)
            nif->skip(12); // Unknown

        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 2) && nif->get<bool>() && hasData)
            nif->readVector(mRotationSpeeds, mNumVertices);

        if (nif->getVersion() != NIFStream::generateVersion(20, 2, 0, 7) || nif->getBethVersion() == 0)
        {
            nif->read(mNumAddedParticles);
            nif->read(mAddedParticlesBase);
        }
    }

    void NiMeshPSysData::read(NIFStream* nif)
    {
        NiPSysData::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 2, 0, 0))
        {
            nif->read(mDefaultPoolSize);
            nif->read(mFillPoolsOnLoad);
            nif->readVector(mGenerations, nif->get<uint32_t>());
        }
        mParticleMeshes.read(nif);
    }

    void NiMeshPSysData::post(Reader& nif)
    {
        NiPSysData::post(nif);

        mParticleMeshes.post(nif);
    }

    void BSStripPSysData::read(NIFStream* nif)
    {
        NiPSysData::read(nif);

        nif->read(mMaxPointCount);
        nif->read(mStartCapSize);
        nif->read(mEndCapSize);
        nif->read(mDoZPrepass);
    }

    void NiPSysModifier::read(NIFStream* nif)
    {
        nif->read(mName);
        mOrder = static_cast<NiPSysModifierOrder>(nif->get<uint32_t>());
        mTarget.read(nif);
        nif->read(mActive);
    }

    void NiPSysModifier::post(Reader& nif)
    {
        mTarget.post(nif);
    }

    void NiPSysAgeDeathModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mSpawnOnDeath);
        mSpawnModifier.read(nif);
    }

    void NiPSysAgeDeathModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mSpawnModifier.post(nif);
    }

    void NiPSysBombModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mBombObject.read(nif);
        nif->read(mBombAxis);
        nif->read(mRange);
        nif->read(mStrength);
        mDecayType = static_cast<DecayType>(nif->get<uint32_t>());
        mSymmetryType = static_cast<SymmetryType>(nif->get<uint32_t>());
    }

    void NiPSysBombModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mBombObject.post(nif);
    }

    void NiPSysBoundUpdateModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mUpdateSkip);
    }

    void NiPSysColorModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mData.read(nif);
    }

    void NiPSysColorModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mData.post(nif);
    }

    void NiPSysDragModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mDragObject.read(nif);
        nif->read(mDragAxis);
        nif->read(mPercentage);
        nif->read(mRange);
        nif->read(mRangeFalloff);
    }

    void NiPSysDragModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mDragObject.post(nif);
    }

    void NiPSysGravityModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mGravityObject.read(nif);
        nif->read(mGravityAxis);
        nif->read(mDecay);
        nif->read(mStrength);
        mForceType = static_cast<ForceType>(nif->get<uint32_t>());
        nif->read(mTurbulence);
        nif->read(mTurbulenceScale);

        if (nif->getBethVersion() >= 17)
            nif->read(mWorldAligned);
    }

    void NiPSysGravityModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mGravityObject.post(nif);
    }

    void NiPSysGrowFadeModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mGrowTime);
        nif->read(mGrowGeneration);
        nif->read(mFadeTime);
        nif->read(mFadeGeneration);
        if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS
            && nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO3)
            nif->read(mBaseScale);
    }

    void NiPSysMeshUpdateModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        readRecordList(nif, mMeshes);
    }

    void NiPSysMeshUpdateModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        postRecordList(nif, mMeshes);
    }

    void NiPSysRotationModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mRotationSpeed);

        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 2))
        {
            nif->read(mRotationSpeedVariation);

            if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO4)
                nif->skip(17); // Unknown

            nif->read(mRotationAngle);
            nif->read(mRotationAngleVariation);
            nif->read(mRandomRotSpeedSign);
        }

        nif->read(mRandomAxis);
        nif->read(mAxis);
    }

    void NiPSysSpawnModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mNumSpawnGenerations);
        nif->read(mPercentageSpawned);
        nif->read(mMinNumToSpawn);
        nif->read(mMaxNumToSpawn);
        nif->read(mSpawnSpeedVariation);
        nif->read(mSpawnDirVariation);
        nif->read(mLifespan);
        nif->read(mLifespanVariation);
    }

    void BSParentVelocityModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mDamping);
    }

    void BSPSysHavokUpdateModifier::read(NIFStream* nif)
    {
        NiPSysMeshUpdateModifier::read(nif);

        mModifier.read(nif);
    }

    void BSPSysHavokUpdateModifier::post(Reader& nif)
    {
        NiPSysMeshUpdateModifier::post(nif);

        mModifier.post(nif);
    }

    void BSPSysInheritVelocityModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mInheritObject.read(nif);
        nif->read(mInheritChance);
        nif->read(mVelocityMult);
        nif->read(mVelcoityVariation);
    }

    void BSPSysInheritVelocityModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mInheritObject.post(nif);
    }

    void BSPSysLODModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mLODStartDistance);
        nif->read(mLODEndDistance);
        nif->read(mEndEmitScale);
        nif->read(mEndSize);
    }

    void BSPSysRecycleBoundModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mBoundOffset);
        nif->read(mBoundExtents);
        mBoundObject.read(nif);
    }

    void BSPSysRecycleBoundModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mBoundObject.post(nif);
    }

    void BSPSysScaleModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->readVector(mScales, nif->get<uint32_t>());
    }

    void BSPSysSimpleColorModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mFadeInPercent);
        nif->read(mFadeOutPercent);
        nif->read(mColor1EndPercent);
        nif->read(mColor1StartPercent);
        nif->read(mColor2EndPercent);
        nif->read(mColor2StartPercent);
        nif->readVector(mColors, 3);
        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            nif->skip(52); // Unknown
    }

    void BSPSysStripUpdateModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mUpdateDeltaTime);
    }

    void BSPSysSubTexModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mStartFrame);
        nif->read(mStartFrameFudge);
        nif->read(mEndFrame);
        nif->read(mLoopStartFrame);
        nif->read(mLoopStartFrameFudge);
        nif->read(mFrameCount);
        nif->read(mFrameCountFudge);
    }

    void BSWindModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mStrength);
    }

    void NiPSysEmitter::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mSpeed);
        nif->read(mSpeedVariation);
        nif->read(mDeclination);
        nif->read(mDeclinationVariation);
        nif->read(mPlanarAngle);
        nif->read(mPlanarAngleVariation);
        nif->read(mInitialColor);
        nif->read(mInitialRadius);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 4, 0, 1))
            nif->read(mRadiusVariation);
        nif->read(mLifespan);
        nif->read(mLifespanVariation);
    }

    void NiPSysVolumeEmitter::read(NIFStream* nif)
    {
        NiPSysEmitter::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            mEmitterObject.read(nif);
    }

    void NiPSysVolumeEmitter::post(Reader& nif)
    {
        NiPSysEmitter::post(nif);

        mEmitterObject.post(nif);
    }

    void NiPSysBoxEmitter::read(NIFStream* nif)
    {
        NiPSysVolumeEmitter::read(nif);

        nif->read(mWidth);
        nif->read(mHeight);
        nif->read(mDepth);
    }

    void NiPSysCylinderEmitter::read(NIFStream* nif)
    {
        NiPSysVolumeEmitter::read(nif);

        nif->read(mRadius);
        nif->read(mHeight);
    }

    void NiPSysMeshEmitter::read(NIFStream* nif)
    {
        NiPSysEmitter::read(nif);

        readRecordList(nif, mEmitterMeshes);

        nif->read(mInitialVelocityType);
        nif->read(mEmissionType);
        nif->read(mEmissionAxis);
    }

    void NiPSysMeshEmitter::post(Reader& nif)
    {
        NiPSysEmitter::post(nif);

        postRecordList(nif, mEmitterMeshes);
    }

    void NiPSysSphereEmitter::read(NIFStream* nif)
    {
        NiPSysVolumeEmitter::read(nif);

        nif->read(mRadius);
    }

    void NiPSysModifierCtlr::read(NIFStream* nif)
    {
        NiSingleInterpController::read(nif);

        nif->read(mModifierName);
    }

    void NiPSysEmitterCtlr::read(NIFStream* nif)
    {
        NiPSysModifierCtlr::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
        else
            mVisInterpolator.read(nif);
    }

    void NiPSysEmitterCtlr::post(Reader& nif)
    {
        NiPSysModifierCtlr::post(nif);

        mData.post(nif);
        mVisInterpolator.post(nif);
    }

    void BSPSysMultiTargetEmitterCtlr::read(NIFStream* nif)
    {
        NiPSysEmitterCtlr::read(nif);

        nif->read(mMaxEmitters);
        mMasterPSys.read(nif);
    }

    void BSPSysMultiTargetEmitterCtlr::post(Reader& nif)
    {
        NiPSysEmitterCtlr::post(nif);

        mMasterPSys.post(nif);
    }

    void NiPSysEmitterCtlrData::read(NIFStream* nif)
    {
        // TODO: this is not used in the official files and needs verification
        mFloatKeyList = std::make_shared<FloatKeyMap>();
        mFloatKeyList->read(nif);
        mVisKeyList = std::make_shared<BoolKeyMap>();
        nif->readVectorOfRecords<uint32_t>(readKeyMapPair<float, bool>, mVisKeyList->mKeys);
    }

    void NiPSysCollider::read(NIFStream* nif)
    {
        nif->read(mBounce);
        nif->read(mCollideSpawn);
        nif->read(mCollideDie);
        mSpawnModifier.read(nif);
        mParent.read(nif);
        mNextCollider.read(nif);
        mColliderObject.read(nif);
    }

    void NiPSysCollider::post(Reader& nif)
    {
        mSpawnModifier.post(nif);
        mParent.post(nif);
        mNextCollider.post(nif);
        mColliderObject.post(nif);
    }

    void NiPSysColliderManager::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mCollider.read(nif);
    }

    void NiPSysColliderManager::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mCollider.post(nif);
    }

    void NiPSysSphericalCollider::read(NIFStream* nif)
    {
        NiPSysCollider::read(nif);

        nif->read(mRadius);
    }

    void NiPSysPlanarCollider::read(NIFStream* nif)
    {
        NiPSysCollider::read(nif);

        nif->read(mWidth);
        nif->read(mHeight);
        nif->read(mXAxis);
        nif->read(mYAxis);
    }

}
