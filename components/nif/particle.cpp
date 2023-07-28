#include "particle.hpp"
#include <components/nif/exception.hpp>

namespace Nif
{
    void NiParticlesData::read(NIFStream* nif)
    {
        bool isBS202 = nif->getVersion() == NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() > 0;
        // if (nif->getVersion() < NIFFile::NIFVersion::VER_BGS)
        NiGeometryData::read(nif);

        // Should always match the number of vertices
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_MW)
            nif->read(numParticles);

        float particleRadius;
        if (nif->getVersion() <= NIFStream::generateVersion(10, 0, 1, 0))
        {
            nif->read(particleRadius);
            particleRadii.resize(vertices.size());
            std::fill(particleRadii.begin(), particleRadii.end(), particleRadius);
        }
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            bool hasRadii = nif->getBoolean();
            // nif->read(hasRadii);
            if (hasRadii && !isBS202)
                nif->readVector(particleRadii, vertices.size());
        }
        activeCount = nif->getUShort();

        // Particle sizes
        bool hasSizes;
        hasSizes = nif->getBoolean();
        if (hasSizes && !isBS202)
            nif->readVector(sizes, vertices.size());

        bool hasRotations = false;
        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
        {
            hasRotations = nif->getBoolean();
            if (hasRotations && !isBS202)
            {
                nif->getQuaternions(mRotations, vertices.size());
            }
        }

        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 4))
        {
            bool hasRotationAngles, hasRotationAxes;

            // nif->read(hasRotationAngles);
            hasRotationAngles = nif->getBoolean();
            if (hasRotationAngles && !isBS202)
                nif->readVector(mRotationAngles, vertices.size());

            // nif->read(hasRotationAxes);
            hasRotationAxes = nif->getBoolean();
            if (hasRotationAxes && !isBS202)
            {
                nif->getVector3s(mRotationAxes, vertices.size());
            }
        }

        bool hasTexIndices = false;
        if (isBS202)
            nif->read(hasTexIndices);

        unsigned int subtexOffsetNum = 0;
        if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS)
        {
            if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
                nif->read(subtexOffsetNum);
            else if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3 && isBS202)
                subtexOffsetNum = nif->get<char>();
        }

        if (isBS202)
            nif->getVector4s(mSubtexOffsets, subtexOffsetNum);

        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3
            && nif->getVersion() == NIFFile::NIFVersion::VER_BGS)
        {
            nif->read(mAspectRatio);

            nif->read(mAspectFlags);

            nif->read(mAspect2);
            nif->read(mSpeed1);
            nif->read(mSpeed2);
        }
    }

    void NiRotatingParticlesData::read(NIFStream* nif)
    {
        NiParticlesData::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(4, 2, 2, 0) && nif->getBoolean())
            nif->getQuaternions(mRotations, vertices.size());
    }

    void NiParticleInfo::read(NIFStream* nif)
    {
        nif->read(mVelocity);
        if (nif->getVersion() <= NIFStream::generateVersion(10, 4, 0, 1))
            nif->read(mRotation);
        nif->read(mAge);
        nif->read(mLifeSpan);
        nif->read(mLastUpdate);
        nif->read(mSpawnGen);
        nif->read(mCode);
    }

    void NiPSysData::read(NIFStream* nif)
    {
        bool isBS202 = nif->getVersion() == NIFStream::generateVersion(20, 2, 0, 7) && nif->getBethVersion() > 0;
        NiPSysDataFlag = true;
        NiParticlesData::read(nif);

        if (nif->getVersion() != NIFStream::generateVersion(20, 2, 0, 7))
        {
            mParticleInfo.resize(vertices.size());
            for (unsigned long i = 0; i < vertices.size(); i++)
            {
                NiParticleInfo temp;
                temp.read(nif);
                mParticleInfo[i] = temp;
            }
        }

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_F76)
            nif->skip(sizeof(float) * 3);

        if (nif->getVersion() == NIFStream::generateVersion(20, 2, 4, 7))
            nif->skip(1);

        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 2))
        {
            bool hasRotationSpeed;
            nif->read(hasRotationSpeed);
            if (hasRotationSpeed && !isBS202)
            {
                mRotationSpeeds.resize(vertices.size());
                nif->readVector(mRotationSpeeds, vertices.size());
            }
        }

        if (!isBS202)
        {
            nif->read(mParticlesAddedNum);
            nif->read(mParticlesBase);
        }

        if (nif->getVersion() == NIFStream::generateVersion(20, 2, 4, 7))
            nif->skip(1);
    }

    void NiParticleSystem::read(NIFStream* nif)
    {
        NiParticleSystemFlag = true;
        NiParticles::read(nif);
        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_SSE)
            mVertexDesc.read(nif);

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_SKY)
        {
            nif->read(mFarBegin);
            nif->read(mFarEnd);
            nif->read(mNearBegin);
            nif->read(mNearEnd);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_SSE)
            mNiPSysData.read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            nif->read(mWorldSpace);
            unsigned int modifierNum;
            nif->read(modifierNum);
            mModifiers.resize(modifierNum);

            for (auto& modifier : mModifiers)
            {
                modifier.read(nif);
            }
        }
    }

    void NiParticleSystem::post(Reader& nif)
    {
        NiParticles::post(nif);
        mNiPSysData.post(nif);
        for (auto ptr : mModifiers)
        {
            ptr.post(nif);
        }
    }

    void NiPSysModifier::read(NIFStream* nif)
    {
        mName = nif->getString();
        nif->read(mOrder);
        mTarget.read(nif);
        nif->read(mActive);
    }

    void NiPSysModifier::post(Reader& nif)
    {
        mTarget.post(nif);
    }

    void NiPSysModifierCtlr::read(NIFStream* nif)
    {
        NiSingleInterpController::read(nif);
        mModifierName = nif->getString();
    }

    void NiPSysEmitterCtlr::read(NIFStream* nif)
    {
        NiPSysModifierCtlr::read(nif);
        if (nif->getVersion() > NIFStream::generateVersion(10, 1, 0, 103))
            mVisibilityInterpolator.read(nif);
        // TODO: Handle pre 10.1.0.103
    }

    void NiPSysEmitterCtlr::post(Reader& nif)
    {
        NiPSysModifierCtlr::post(nif);
        if (nif.getVersion() > NIFStream::generateVersion(10, 1, 0, 103))
            mVisibilityInterpolator.post(nif);
    }

    void NiPSysAgeDeathModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mSpawnOnDeath);
        mSpawnModifier.read(nif);
    }

    void NiPSysAgeDeathModifier::post(Reader& nif)
    {
        mSpawnModifier.post(nif);
    }

    void NiPSysSpawnModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mNumSpawnGens);
        nif->read(mPercentSpawned);
        nif->read(mMinSpawnNum);
        nif->read(mMaxSpawnNum);
        nif->read(mSpawnSpeedVariation);
        nif->read(mSpawnDirVariation);
        nif->read(mLifeSpan);
        nif->read(mLifeSpanVariation);
    }

    void BSPSysLODModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mBeginDist);
        nif->read(mEndDist);
        nif->read(mEndEmitScale);
        nif->read(mEndSize);
    }

    void NiPSysCylinderEmitter::read(NIFStream* nif)
    {
        NiPSysVolumeEmitter::read(nif);

        nif->read(mRadius);
        nif->read(mHeight);
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

        if (nif.getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            mEmitterObject.post(nif);
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
        // nif->skip(sizeof(float) * 2);
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
        nif->getVector4s(mColors, 3);
        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_F76)
            nif->skip(sizeof(unsigned short) * 26);
    }

    void NiPSysRotationModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mRotationSpeed);
        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 2))
            nif->read(mRotationSpeedVariation);

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_F76)
            throw Nif::Exception("Fallout76 is unsupported: ", nif->getFile().getFilename());

        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 2))
        {
            nif->read(mRotationAngle);
            nif->read(mRotationAngleVariation);
            mRandRotSpeedSign = nif->getBoolean();
        }

        mRandAxis = nif->getBoolean();
        nif->read(mAxis);
    }

    void BSPSysScaleModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        unsigned int numScales;
        nif->read(numScales);
        nif->readVector(mScales, numScales);
    }

    void NiPSysGravityModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mGravityObject.read(nif);
        nif->read(mGravityAxis);
        nif->read(mDecay);
        nif->read(mStrength);
        nif->read(mForceType);
        nif->read(mTurbulence);
        nif->read(mTurbulenceScale);

        if (nif->getBethVersion() > 16)
            mWorldAligned = nif->getBoolean();
    }

    void NiPSysGravityModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mGravityObject.post(nif);
    }

    void NiPSysBoundUpdateModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        nif->read(mUpdateSkip);
    }

    void NiPSysModifierActiveCtlr::read(NIFStream* nif)
    {
        NiPSysModifierCtlr::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mNiVisData.read(nif);
    }

    void NiPSysModifierActiveCtlr::post(Reader& nif)
    {
        NiPSysModifierCtlr::post(nif);

        if (nif.getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mNiVisData.post(nif);
    }

    void NiPSysMeshEmitter::read(NIFStream* nif)
    {
        NiPSysEmitter::read(nif);

        unsigned int meshNum;
        nif->read(meshNum);

        mEmitterMeshes.resize(meshNum);
        for (auto& mesh : mEmitterMeshes)
            mesh.read(nif);

        nif->read(mInitialVelocityType);
        nif->read(mEmissionType);
        nif->read(mEmissionAxis);
    }

    void NiPSysMeshEmitter::post(Reader& nif)
    {
        NiPSysEmitter::post(nif);

        for (auto& mesh : mEmitterMeshes)
            mesh.post(nif);
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

    void NiPSysBombModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mBombObj.read(nif);
        nif->read(mBombAxis);
        nif->read(mDecay);
        nif->read(mDeltaV);
        nif->read(mDecayType);
        nif->read(mSymmetryType);
    }

    void NiPSysBombModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mBombObj.post(nif);
    }

    void NiPSysDragModifier::read(NIFStream* nif)
    {
        NiPSysModifier::read(nif);

        mDragObj.read(nif);
        nif->read(mDragAxis);
        nif->read(mPercentage);
        nif->read(mRange);
        nif->read(mRangeFalloff);
    }

    void NiPSysDragModifier::post(Reader& nif)
    {
        NiPSysModifier::post(nif);

        mDragObj.post(nif);
    }
}
