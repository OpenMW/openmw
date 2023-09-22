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

        bool numRadii = 1;
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
        {
            mParticles.resize(mNumVertices);
            for (NiParticleInfo& info : mParticles)
                info.read(nif);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            nif->skip(12); // Unknown

        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 2) && nif->get<bool>() && hasData)
            nif->readVector(mRotationSpeeds, mNumVertices);

        if (nif->getVersion() != NIFStream::generateVersion(20, 2, 0, 7) || nif->getBethVersion() == 0)
        {
            nif->read(mNumAddedParticles);
            nif->read(mAddedParticlesBase);
        }
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

    void BSPSysLODModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mLODStartDistance);
        nif->read(mLODEndDistance);
        nif->read(mEndEmitScale);
        nif->read(mEndSize);
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

    void NiPSysEmitterCtlrData::read(NIFStream* nif)
    {
        mFloatKeyList = std::make_shared<FloatKeyMap>();
        mVisKeyList = std::make_shared<BoolKeyMap>();
        uint32_t numVisKeys;
        nif->read(numVisKeys);
        for (size_t i = 0; i < numVisKeys; i++)
            mVisKeyList->mKeys[nif->get<float>()].mValue = nif->get<uint8_t>() != 0;
    }

}
