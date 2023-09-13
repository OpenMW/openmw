#include "controller.hpp"

#include "data.hpp"
#include "exception.hpp"
#include "node.hpp"
#include "particle.hpp"
#include "texture.hpp"

namespace Nif
{

    void Controller::read(NIFStream* nif)
    {
        next.read(nif);

        flags = nif->getUShort();
        frequency = nif->getFloat();
        phase = nif->getFloat();
        timeStart = nif->getFloat();
        timeStop = nif->getFloat();
        mTarget.read(nif);
    }

    void Controller::post(Reader& nif)
    {
        next.post(nif);
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
        uint32_t numBlocks;
        nif->read(numBlocks);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 106))
            nif->read(mArrayGrowBy);
        mControlledBlocks.resize(numBlocks);
        for (ControlledBlock& block : mControlledBlocks)
            block.read(nif);
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
        mExtrapolationMode = static_cast<Controller::ExtrapolationMode>(nif->getUInt());
        mFrequency = nif->getFloat();
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
            uint16_t numAnimNotes = 1;
            if (nif->getBethVersion() >= 29)
                nif->read(numAnimNotes);

            nif->skip(4 * numAnimNotes); // BSAnimNotes links
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
        Controller::read(nif);

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

    void NiParticleSystemController::read(NIFStream* nif)
    {
        Controller::read(nif);

        velocity = nif->getFloat();
        velocityRandom = nif->getFloat();
        verticalDir = nif->getFloat();
        verticalAngle = nif->getFloat();
        horizontalDir = nif->getFloat();
        horizontalAngle = nif->getFloat();
        /*normal?*/ nif->getVector3();
        color = nif->getVector4();
        size = nif->getFloat();
        startTime = nif->getFloat();
        stopTime = nif->getFloat();
        nif->getChar();
        emitRate = nif->getFloat();
        lifetime = nif->getFloat();
        lifetimeRandom = nif->getFloat();

        emitFlags = nif->getUShort();
        offsetRandom = nif->getVector3();

        emitter.read(nif);

        /* Unknown Short, 0?
         * Unknown Float, 1.0?
         * Unknown Int, 1?
         * Unknown Int, 0?
         * Unknown Short, 0?
         */
        nif->skip(16);

        numParticles = nif->getUShort();
        activeCount = nif->getUShort();

        particles.resize(numParticles);
        for (size_t i = 0; i < particles.size(); i++)
        {
            particles[i].velocity = nif->getVector3();
            nif->getVector3(); /* unknown */
            particles[i].lifetime = nif->getFloat();
            particles[i].lifespan = nif->getFloat();
            particles[i].timestamp = nif->getFloat();
            nif->getUShort(); /* unknown */
            particles[i].vertex = nif->getUShort();
        }

        nif->getUInt(); /* -1? */
        affectors.read(nif);
        colliders.read(nif);
        nif->getChar();
    }

    void NiParticleSystemController::post(Reader& nif)
    {
        Controller::post(nif);

        emitter.post(nif);
        affectors.post(nif);
        colliders.post(nif);
    }

    void NiMaterialColorController::read(NIFStream* nif)
    {
        NiPoint3InterpController::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            mTargetColor = static_cast<TargetColor>(nif->get<uint16_t>() & 3);
        else
            mTargetColor = static_cast<TargetColor>((flags >> 4) & 3);

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
        Controller::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mLookAtFlags);
        mLookAt.read(nif);
    }

    void NiLookAtController::post(Reader& nif)
    {
        Controller::post(nif);

        mLookAt.post(nif);
    }

    void NiPathController::read(NIFStream* nif)
    {
        Controller::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mPathFlags);
        else
            mPathFlags = (flags >> 16);

        nif->read(mBankDirection);
        nif->read(mMaxBankAngle);
        nif->read(mSmoothing);
        nif->read(mFollowAxis);
        mPathData.read(nif);
        mPercentData.read(nif);
    }

    void NiPathController::post(Reader& nif)
    {
        Controller::post(nif);

        mPathData.post(nif);
        mPercentData.post(nif);
    }

    void NiUVController::read(NIFStream* nif)
    {
        Controller::read(nif);

        nif->read(mUvSet);
        mData.read(nif);
    }

    void NiUVController::post(Reader& nif)
    {
        Controller::post(nif);

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
            nif->read(timeStart);
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

    void bhkBlendController::read(NIFStream* nif)
    {
        Controller::read(nif);

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

    void NiControllerManager::read(NIFStream* nif)
    {
        Controller::read(nif);

        nif->read(mCumulative);
        readRecordList(nif, mSequences);
        mObjectPalette.read(nif);
    }

    void NiControllerManager::post(Reader& nif)
    {
        Controller::post(nif);

        postRecordList(nif, mSequences);
        mObjectPalette.post(nif);
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

}
