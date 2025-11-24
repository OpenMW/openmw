#include "controller.hpp"

#include "data.hpp"
#include "exception.hpp"
#include "node.hpp"
#include "particle.hpp"
#include "texture.hpp"

namespace Nif
{
    namespace
    {
        void readSkinnedShapeGroup(NIFStream& stream, std::vector<NiBoneLODController::SkinInfo>& value)
        {
            stream.readVectorOfRecords<uint32_t>(value);
        }
    }

    void NiTimeController::read(NIFStream* nif)
    {
        mNext.read(nif);
        nif->read(mFlags);
        nif->read(mFrequency);
        nif->read(mPhase);
        nif->read(mTimeStart);
        nif->read(mTimeStop);
        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            mTarget.read(nif);
    }

    void NiTimeController::post(Reader& nif)
    {
        mNext.post(nif);
        mTarget.post(nif);
    }

    void ControlledBlock::read(NIFStream* nif)
    {
        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mTargetName = nif->getSizedString();
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 106))
            mInterpolator.read(nif);
        if (nif->getVersion() <= NIFStream::generateVersion(20, 5, 0, 0))
            mController.read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            return;

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 110))
        {
            mBlendInterpolator.read(nif);
            nif->read(mBlendIndex);
        }
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 106) && nif->getBethVersion() > 0)
            nif->read(mPriority);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 2, 0, 0)
            && nif->getVersion() <= NIFStream::generateVersion(20, 1, 0, 0))
        {
            mStringPalette.read(nif);
            nif->read(mNodeNameOffset);
            nif->read(mPropertyTypeOffset);
            nif->read(mControllerTypeOffset);
            nif->read(mControllerIdOffset);
            nif->read(mInterpolatorIdOffset);
        }
        else
        {
            nif->read(mNodeName);
            nif->read(mPropertyType);
            nif->read(mControllerType);
            nif->read(mControllerId);
            nif->read(mInterpolatorId);
        }
    }

    void ControlledBlock::post(Reader& nif)
    {
        mInterpolator.post(nif);
        mController.post(nif);
        mBlendInterpolator.post(nif);
        mStringPalette.post(nif);
        // TODO: probably should fill the strings with string palette contents here
    }

    void NiSequence::read(NIFStream* nif)
    {
        nif->read(mName);
        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
        {
            nif->read(mAccumRootName);
            mTextKeys.read(nif);
        }
        const uint32_t size = nif->get<uint32_t>();
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 106))
            nif->read(mArrayGrowBy);
        nif->readVectorOfRecords(size, mControlledBlocks);
    }

    void NiSequence::post(Reader& nif)
    {
        mTextKeys.post(nif);
        for (ControlledBlock& block : mControlledBlocks)
            block.post(nif);
    }

    void NiControllerSequence::read(NIFStream* nif)
    {
        NiSequence::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            return;

        nif->read(mWeight);
        mTextKeys.read(nif);
        mExtrapolationMode = static_cast<NiTimeController::ExtrapolationMode>(nif->get<uint32_t>());
        nif->read(mFrequency);
        if (nif->getVersion() <= NIFStream::generateVersion(10, 4, 0, 1))
            nif->read(mPhase);
        nif->read(mStartTime);
        nif->read(mStopTime);
        if (nif->getVersion() == NIFStream::generateVersion(10, 1, 0, 106))
            nif->read(mPlayBackwards);
        mManager.read(nif);
        nif->read(mAccumRootName);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 113)
            && nif->getVersion() <= NIFStream::generateVersion(20, 1, 0, 0))
            mStringPalette.read(nif);
        else if (nif->getVersion() >= NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() >= 24)
        {
            const uint16_t size = nif->getBethVersion() >= 29 ? nif->get<uint16_t>() : 1;
            nif->readVectorOfRecords(size, mAnimNotesList);
        }
    }

    void NiControllerSequence::post(Reader& nif)
    {
        NiSequence::post(nif);

        mManager.post(nif);
        mStringPalette.post(nif);
    }

    void NiInterpController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 104)
            && nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 108))
            nif->read(mManagerControlled);
    }

    void NiSingleInterpController::read(NIFStream* nif)
    {
        NiInterpController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 104))
            mInterpolator.read(nif);
    }

    void NiSingleInterpController::post(Reader& nif)
    {
        NiInterpController::post(nif);

        mInterpolator.post(nif);
    }

    void NiParticleInfo::read(NIFStream* nif)
    {
        nif->read(mVelocity);
        if (nif->getVersion() <= NIFStream::generateVersion(10, 4, 0, 1))
            nif->read(mRotationAxis);
        nif->read(mAge);
        nif->read(mLifespan);
        nif->read(mLastUpdate);
        nif->read(mSpawnGeneration);
        nif->read(mCode);
    }

    void NiParticleSystemController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            nif->read(mSpeed);
        nif->read(mSpeedVariation);
        nif->read(mDeclination);
        nif->read(mDeclinationVariation);
        nif->read(mPlanarAngle);
        nif->read(mPlanarAngleVariation);
        nif->read(mInitialNormal);
        nif->read(mInitialColor);
        nif->read(mInitialSize);
        nif->read(mEmitStartTime);
        nif->read(mEmitStopTime);
        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
        {
            mResetParticleSystem = nif->get<uint8_t>() != 0;
            nif->read(mBirthRate);
        }
        nif->read(mLifetime);
        nif->read(mLifetimeVariation);
        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            nif->read(mEmitFlags);
        nif->read(mEmitterDimensions);
        mEmitter.read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
        {
            nif->read(mNumSpawnGenerations);
            nif->read(mPercentageSpawned);
            nif->read(mSpawnMultiplier);
            nif->read(mSpawnSpeedChaos);
            nif->read(mSpawnDirChaos);
            mParticles.resize(nif->get<uint16_t>());
            nif->read(mNumValid);
            for (NiParticleInfo& particle : mParticles)
                particle.read(nif);
            nif->skip(4); // NiEmitterModifier link
        }
        mModifier.read(nif);
        mCollider.read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 15))
            nif->read(mStaticTargetBound);
    }

    void NiParticleSystemController::post(Reader& nif)
    {
        NiTimeController::post(nif);

        mEmitter.post(nif);
        mModifier.post(nif);
        mCollider.post(nif);
    }

    void NiLightColorController::read(NIFStream* nif)
    {
        NiPoint3InterpController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            mMode = static_cast<Mode>(nif->get<uint16_t>());
        else
            mMode = static_cast<Mode>((mFlags >> 4) & 1);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
    }

    void NiLightColorController::post(Reader& nif)
    {
        NiPoint3InterpController::post(nif);

        mData.post(nif);
    }

    void NiMaterialColorController::read(NIFStream* nif)
    {
        NiPoint3InterpController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            mTargetColor = static_cast<TargetColor>(nif->get<uint16_t>());
        else
            mTargetColor = static_cast<TargetColor>((mFlags >> 4) & 3);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
    }

    void NiMaterialColorController::post(Reader& nif)
    {
        NiPoint3InterpController::post(nif);

        mData.post(nif);
    }

    void NiLookAtController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mLookAtFlags);
        mLookAt.read(nif);
    }

    void NiLookAtController::post(Reader& nif)
    {
        NiTimeController::post(nif);

        mLookAt.post(nif);
    }

    void NiPathController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mPathFlags);
        else
            mPathFlags = (mFlags >> 4);

        nif->read(mBankDirection);
        nif->read(mMaxBankAngle);
        nif->read(mSmoothing);
        nif->read(mFollowAxis);
        mPathData.read(nif);
        mPercentData.read(nif);
    }

    void NiPathController::post(Reader& nif)
    {
        NiTimeController::post(nif);

        mPathData.post(nif);
        mPercentData.post(nif);
    }

    void NiUVController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        nif->read(mUvSet);
        mData.read(nif);
    }

    void NiUVController::post(Reader& nif)
    {
        NiTimeController::post(nif);

        mData.post(nif);
    }

    void NiKeyframeController::read(NIFStream* nif)
    {
        NiSingleInterpController::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
    }

    void NiKeyframeController::post(Reader& nif)
    {
        NiSingleInterpController::post(nif);

        mData.post(nif);
    }

    void NiMultiTargetTransformController::read(NIFStream* nif)
    {
        NiInterpController::read(nif);

        mExtraTargets.resize(nif->get<uint16_t>());
        for (NiAVObjectPtr& extraTarget : mExtraTargets)
            extraTarget.read(nif);
    }

    void NiMultiTargetTransformController::post(Reader& nif)
    {
        NiInterpController::post(nif);

        postRecordList(nif, mExtraTargets);
    }

    void NiAlphaController::read(NIFStream* nif)
    {
        NiFloatInterpController::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
    }

    void NiAlphaController::post(Reader& nif)
    {
        NiFloatInterpController::post(nif);

        mData.post(nif);
    }

    void NiRollController::read(NIFStream* nif)
    {
        NiSingleInterpController::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
    }

    void NiRollController::post(Reader& nif)
    {
        NiSingleInterpController::post(nif);

        mData.post(nif);
    }

    void NiGeomMorpherController::read(NIFStream* nif)
    {
        NiInterpController::read(nif);

        if (nif->getVersion() >= NIFFile::NIFVersion::VER_OB_OLD)
            mUpdateNormals = nif->get<uint16_t>() & 1;
        mData.read(nif);

        if (nif->getVersion() < NIFFile::NIFVersion::VER_MW)
            return;

        mAlwaysActive = nif->get<uint8_t>() != 0;

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 105))
            return;

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            readRecordList(nif, mInterpolators);
            if (nif->getVersion() >= NIFStream::generateVersion(10, 2, 0, 0) && nif->getBethVersion() >= 10)
                nif->skip(4 * nif->get<uint32_t>()); // Unknown
            return;
        }

        mInterpolators.resize(nif->get<uint32_t>());
        mWeights.resize(mInterpolators.size());
        for (size_t i = 0; i < mInterpolators.size(); i++)
        {
            mInterpolators[i].read(nif);
            nif->read(mWeights[i]);
        }
    }

    void NiGeomMorpherController::post(Reader& nif)
    {
        NiInterpController::post(nif);

        mData.post(nif);
        postRecordList(nif, mInterpolators);
    }

    void NiVisController::read(NIFStream* nif)
    {
        NiBoolInterpController::read(nif);

        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
    }

    void NiVisController::post(Reader& nif)
    {
        NiBoolInterpController::post(nif);

        mData.post(nif);
    }

    void NiFlipController::read(NIFStream* nif)
    {
        NiFloatInterpController::read(nif);

        mTexSlot = static_cast<NiTexturingProperty::TextureType>(nif->get<uint32_t>());
        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
        {
            nif->read(mTimeStart);
            nif->read(mDelta);
        }
        readRecordList(nif, mSources);
    }

    void NiFlipController::post(Reader& nif)
    {
        NiFloatInterpController::post(nif);

        postRecordList(nif, mSources);
    }

    void NiTextureTransformController::read(NIFStream* nif)
    {
        NiFloatInterpController::read(nif);

        nif->read(mShaderMap);
        mTexSlot = static_cast<NiTexturingProperty::TextureType>(nif->get<uint32_t>());
        nif->read(mTransformMember);
        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
    }

    void NiTextureTransformController::post(Reader& nif)
    {
        NiFloatInterpController::post(nif);

        mData.post(nif);
    }

    void NiBoneLODController::SkinInfo::read(NIFStream* nif)
    {
        mShape.read(nif);
        mSkin.read(nif);
    }

    void NiBoneLODController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        nif->read(mLOD);
        const uint32_t nodeGroupsCount = nif->get<uint32_t>();
        mNodeGroups.reserve(nodeGroupsCount);
        nif->read(mNumNodeGroups);
        for (uint32_t i = 0; i < nodeGroupsCount; ++i)
            readRecordList(nif, mNodeGroups.emplace_back());

        if (nif->getBethVersion() != 0 || nif->getVersion() < NIFStream::generateVersion(4, 2, 2, 0))
            return;

        nif->readVectorOfRecords<uint32_t>(readSkinnedShapeGroup, mSkinnedShapeGroups);
        readRecordList(nif, mShapeGroups);
    }

    void NiBoneLODController::post(Reader& nif)
    {
        NiTimeController::post(nif);

        for (NiAVObjectList& group : mNodeGroups)
            postRecordList(nif, group);

        for (std::vector<SkinInfo>& group : mSkinnedShapeGroups)
        {
            for (SkinInfo& info : group)
            {
                info.mShape.post(nif);
                info.mSkin.post(nif);
            }
        }
        postRecordList(nif, mShapeGroups);
    }

    void bhkBlendController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        uint32_t numKeys;
        nif->read(numKeys);
        // Is this possible?
        if (numKeys != 0)
            throw Nif::Exception(
                "Unsupported keys in bhkBlendController " + std::to_string(recIndex), nif->getFile().getFilename());
    }

    void BSEffectShaderPropertyFloatController::read(NIFStream* nif)
    {
        NiFloatInterpController::read(nif);

        nif->read(mControlledVariable);
    }

    void BSEffectShaderPropertyColorController::read(NIFStream* nif)
    {
        NiPoint3InterpController::read(nif);

        nif->read(mControlledColor);
    }

    void BSKeyframeController::read(NIFStream* nif)
    {
        NiKeyframeController::read(nif);

        mData2.read(nif);
    }

    void BSKeyframeController::post(Reader& nif)
    {
        NiKeyframeController::post(nif);

        mData2.post(nif);
    }

    void BSLagBoneController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        nif->read(mLinearVelocity);
        nif->read(mLinearRotation);
        nif->read(mMaximumDistance);
    }

    void BSProceduralLightningController::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        mGenerationInterp.read(nif);
        mMutationInterp.read(nif);
        mSubdivisionInterp.read(nif);
        mNumBranchesInterp.read(nif);
        mNumBranchesVarInterp.read(nif);
        mLengthInterp.read(nif);
        mLengthVarInterp.read(nif);
        mWidthInterp.read(nif);
        mArcOffsetInterp.read(nif);
        nif->read(mSubdivisions);
        nif->read(mNumBranches);
        nif->read(mNumBranchesVar);
        nif->read(mLength);
        nif->read(mLengthVar);
        nif->read(mWidth);
        nif->read(mChildWidthMult);
        nif->read(mArcOffset);
        nif->read(mFadeMainBolt);
        nif->read(mFadeChildBolts);
        nif->read(mAnimateArcOffset);
        mShaderProperty.read(nif);
    }

    void BSProceduralLightningController::post(Reader& nif)
    {
        NiTimeController::post(nif);

        mGenerationInterp.post(nif);
        mMutationInterp.post(nif);
        mSubdivisionInterp.post(nif);
        mNumBranchesInterp.post(nif);
        mNumBranchesVarInterp.post(nif);
        mLengthInterp.post(nif);
        mLengthVarInterp.post(nif);
        mWidthInterp.post(nif);
        mArcOffsetInterp.post(nif);
        mShaderProperty.post(nif);
    }

    void NiControllerManager::read(NIFStream* nif)
    {
        NiTimeController::read(nif);

        nif->read(mCumulative);
        readRecordList(nif, mSequences);
        mObjectPalette.read(nif);
    }

    void NiControllerManager::post(Reader& nif)
    {
        NiTimeController::post(nif);

        postRecordList(nif, mSequences);
        mObjectPalette.post(nif);
    }

    void NiExtraDataController::read(NIFStream* nif)
    {
        NiSingleInterpController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 2, 0, 0))
            nif->read(mExtraDataName);
    }

    void NiFloatExtraDataController::read(NIFStream* nif)
    {
        NiExtraDataController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 104))
            return;

        // Unknown
        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 0))
        {
            uint8_t numExtraBytes;
            nif->read(numExtraBytes);
            nif->skip(7);
            nif->skip(numExtraBytes);
        }

        mData.read(nif);
    }

    void NiFloatExtraDataController::post(Reader& nif)
    {
        NiExtraDataController::post(nif);

        mData.post(nif);
    }

    void NiFloatsExtraDataController::read(NIFStream* nif)
    {
        NiExtraDataController::read(nif);

        nif->read(mFloatsExtraDataIndex);
        if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
            mData.read(nif);
    }

    void NiFloatsExtraDataController::post(Reader& nif)
    {
        NiExtraDataController::post(nif);

        mData.post(nif);
    }

    void NiFloatsExtraDataPoint3Controller::read(NIFStream* nif)
    {
        NiExtraDataController::read(nif);

        nif->read(mFloatsExtraDataIndex);
    }

    void NiPathInterpolator::read(NIFStream* nif)
    {
        nif->read(mFlags);
        nif->read(mBankDirection);
        nif->read(mMaxBankAngle);
        nif->read(mSmoothing);
        nif->read(mFollowAxis);
        mPathData.read(nif);
        mPercentData.read(nif);
    }

    void NiPathInterpolator::post(Reader& nif)
    {
        mPathData.post(nif);
        mPercentData.post(nif);
    }

    void NiLookAtInterpolator::read(NIFStream* nif)
    {
        nif->read(mLookAtFlags);
        mLookAt.read(nif);
        nif->read(mLookAtName);
        if (nif->getVersion() <= NIFStream::generateVersion(20, 4, 0, 12))
            nif->read(mTransform);
        mTranslation.read(nif);
        mRoll.read(nif);
        mScale.read(nif);
    }

    void NiLookAtInterpolator::post(Reader& nif)
    {
        mLookAt.post(nif);
        mTranslation.post(nif);
        mRoll.post(nif);
        mScale.post(nif);
    }

    void NiBlendInterpolator::read(NIFStream* nif)
    {
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 112))
        {
            nif->read(mFlags);
            mItems.resize(nif->get<uint8_t>());
            nif->read(mWeightThreshold);
            if (!(mFlags & Flag_ManagerControlled))
            {
                mInterpCount = nif->get<uint8_t>();
                mSingleIndex = nif->get<uint8_t>();
                mHighPriority = nif->get<int8_t>();
                mNextHighPriority = nif->get<int8_t>();
                nif->read(mSingleTime);
                nif->read(mHighWeightsSum);
                nif->read(mNextHighWeightsSum);
                nif->read(mHighEaseSpinner);
                for (Item& item : mItems)
                    item.read(nif);
            }
            return;
        }

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 110))
        {
            mItems.resize(nif->get<uint8_t>());
            for (Item& item : mItems)
                item.read(nif);
            if (nif->get<bool>())
                mFlags |= Flag_ManagerControlled;
            nif->read(mWeightThreshold);
            if (nif->get<bool>())
                mFlags |= Flag_OnlyUseHighestWeight;
            mInterpCount = nif->get<uint8_t>();
            mSingleIndex = nif->get<uint8_t>();
            mSingleInterpolator.read(nif);
            nif->read(mSingleTime);
            mHighPriority = nif->get<int8_t>();
            mNextHighPriority = nif->get<int8_t>();
            return;
        }

        mItems.resize(nif->get<uint16_t>());
        nif->read(mArrayGrowBy);
        for (Item& item : mItems)
            item.read(nif);
        if (nif->get<bool>())
            mFlags |= Flag_ManagerControlled;
        nif->read(mWeightThreshold);
        if (nif->get<bool>())
            mFlags |= Flag_OnlyUseHighestWeight;
        nif->read(mInterpCount);
        nif->read(mSingleIndex);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 108))
        {
            mSingleInterpolator.read(nif);
            nif->read(mSingleTime);
        }
        nif->read(mHighPriority);
        nif->read(mNextHighPriority);
    }

    void NiBlendInterpolator::post(Reader& nif)
    {
        for (Item& item : mItems)
            item.post(nif);
        mSingleInterpolator.post(nif);
    }

    void NiBlendInterpolator::Item::read(NIFStream* nif)
    {
        mInterpolator.read(nif);
        nif->read(mWeight);
        nif->read(mNormalizedWeight);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 110))
            mPriority = nif->get<int8_t>();
        else
            nif->read(mPriority);
        nif->read(mEaseSpinner);
    }

    void NiBlendInterpolator::Item::post(Reader& nif)
    {
        mInterpolator.post(nif);
    }

    void NiBSplineInterpolator::read(NIFStream* nif)
    {
        nif->read(mStartTime);
        nif->read(mStopTime);
        mSplineData.read(nif);
        mBasisData.read(nif);
    }

    void NiBSplineInterpolator::post(Reader& nif)
    {
        mSplineData.post(nif);
        mBasisData.post(nif);
    }

    void NiBSplineFloatInterpolator::read(NIFStream* nif)
    {
        NiBSplineInterpolator::read(nif);

        nif->read(mValue);
        nif->read(mHandle);
    }

    void NiBSplineCompFloatInterpolator::read(NIFStream* nif)
    {
        NiBSplineFloatInterpolator::read(nif);

        nif->read(mOffset);
        nif->read(mHalfRange);
    }

    void NiBSplinePoint3Interpolator::read(NIFStream* nif)
    {
        NiBSplineInterpolator::read(nif);

        nif->read(mValue);
        nif->read(mHandle);
    }

    void NiBSplineCompPoint3Interpolator::read(NIFStream* nif)
    {
        NiBSplinePoint3Interpolator::read(nif);

        nif->read(mOffset);
        nif->read(mHalfRange);
    }

    void NiBSplineTransformInterpolator::read(NIFStream* nif)
    {
        NiBSplineInterpolator::read(nif);

        nif->read(mValue);
        nif->read(mTranslationHandle);
        nif->read(mRotationHandle);
        nif->read(mScaleHandle);
    }

    void NiBSplineCompTransformInterpolator::read(NIFStream* nif)
    {
        NiBSplineTransformInterpolator::read(nif);

        nif->read(mTranslationOffset);
        nif->read(mTranslationHalfRange);
        nif->read(mRotationOffset);
        nif->read(mRotationHalfRange);
        nif->read(mScaleOffset);
        nif->read(mScaleHalfRange);
    }

    void BSTreadTransform::read(NIFStream* nif)
    {
        nif->read(mName);
        nif->read(mTransform1);
        nif->read(mTransform2);
    }

    void BSTreadTransfInterpolator::read(NIFStream* nif)
    {
        nif->readVectorOfRecords<uint32_t>(mTransforms);
        mData.read(nif);
    }

    void BSTreadTransfInterpolator::post(Reader& nif)
    {
        mData.post(nif);
    }

}
