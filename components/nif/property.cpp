#include "property.hpp"

#include "data.hpp"
#include "texture.hpp"

namespace Nif
{

    void NiTextureTransform::read(NIFStream* nif)
    {
        nif->read(mOffset);
        nif->read(mScale);
        nif->read(mRotation);
        mTransformMethod = static_cast<Method>(nif->get<uint32_t>());
        nif->read(mOrigin);
    }

    void NiTexturingProperty::Texture::read(NIFStream* nif)
    {
        nif->read(mEnabled);
        if (!mEnabled)
            return;

        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            mSourceTexture.read(nif);

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            nif->read(mClamp);
            nif->read(mFilter);
        }
        else
        {
            uint16_t flags;
            nif->read(flags);
            mClamp = flags & 0xF;
            mFilter = (flags >> 4) & 0xF;
        }

        if (nif->getVersion() >= NIFStream::generateVersion(20, 5, 0, 4))
            nif->read(mMaxAnisotropy);

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
            nif->read(mUVSet);

        // PS2 filtering settings
        if (nif->getVersion() <= NIFStream::generateVersion(10, 4, 0, 1))
            nif->skip(4);

        if (nif->getVersion() <= NIFStream::generateVersion(4, 1, 0, 12))
            nif->skip(2); // Unknown

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            nif->read(mHasTransform);
            if (mHasTransform)
                mTransform.read(nif);
        }
    }

    void NiTexturingProperty::Texture::post(Reader& nif)
    {
        mSourceTexture.post(nif);
    }

    void NiTexturingProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD
            || nif->getVersion() >= NIFStream::generateVersion(20, 1, 0, 2))
            nif->read(mFlags);
        if (nif->getVersion() <= NIFStream::generateVersion(20, 1, 0, 1))
            mApplyMode = static_cast<ApplyMode>(nif->get<uint32_t>());

        const uint32_t texturesSize = nif->get<uint32_t>();

        mTextures.reserve(texturesSize);
        for (size_t i = 0; i < texturesSize; i++)
        {
            mTextures.emplace_back().read(nif);

            if (i == 5 && mTextures[5].mEnabled)
            {
                nif->read(mEnvMapLumaBias);
                nif->read(mBumpMapMatrix);
            }
            else if (i == 7 && mTextures[7].mEnabled && nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 5))
                nif->read(mParallaxOffset);
        }

        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
        {
            const uint32_t sharedTexturesSize = nif->get<uint32_t>();
            mShaderTextures.reserve(sharedTexturesSize);
            mShaderIds.reserve(sharedTexturesSize);
            for (size_t i = 0; i < sharedTexturesSize; i++)
            {
                mShaderTextures.emplace_back().read(nif);
                uint32_t id = 0;
                if (mShaderTextures[i].mEnabled)
                    nif->read(id);
                mShaderIds.push_back(id);
            }
        }
    }

    void NiTexturingProperty::post(Reader& nif)
    {
        NiProperty::post(nif);

        for (Texture& tex : mTextures)
            tex.post(nif);
        for (Texture& tex : mShaderTextures)
            tex.post(nif);
    }

    void BSSPParallaxParams::read(NIFStream* nif)
    {
        nif->read(mMaxPasses);
        nif->read(mScale);
    }

    void BSSPRefractionParams::read(NIFStream* nif)
    {
        nif->read(mStrength);
        nif->read(mPeriod);
    }

    void BSShaderProperty::read(NIFStream* nif)
    {
        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_F76 && recType == RC_BSLightingShaderProperty)
            nif->read(mType);

        NiShadeProperty::read(nif);

        if (nif->getUserVersion() <= 11)
        {
            nif->read(mType);
            nif->read(mShaderFlags1);
            nif->read(mShaderFlags2);
            nif->read(mEnvMapScale);
            return;
        }

        if (!mName.empty() && nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            return;

        if (nif->getBethVersion() <= 131)
        {
            nif->read(mShaderFlags1);
            nif->read(mShaderFlags2);
        }
        else
        {
            if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76 && recType == RC_BSLightingShaderProperty)
            {
                nif->read(mType);

                // Remap FO76+ shader types to FO4 system so that we can actually use them
                // TODO: NifTools spec doesn't do anything about the misplaced EyeEnvmap. Bug or feature?
                switch (mType)
                {
                    case 3:
                        mType = static_cast<uint32_t>(BSLightingShaderType::ShaderType_FaceTint);
                        break;
                    case 4:
                        mType = static_cast<uint32_t>(BSLightingShaderType::ShaderType_SkinTint);
                        break;
                    case 5:
                        mType = static_cast<uint32_t>(BSLightingShaderType::ShaderType_HairTint);
                        break;
                    case 12:
                        mType = static_cast<uint32_t>(BSLightingShaderType::ShaderType_EyeEnvmap);
                        break;
                    case 17:
                        mType = static_cast<uint32_t>(BSLightingShaderType::ShaderType_Terrain);
                        break;
                    default:
                        break;
                }
            }

            uint32_t numShaderFlags1 = 0, numShaderFlags2 = 0;
            nif->read(numShaderFlags1);
            if (nif->getBethVersion() >= 152)
                nif->read(numShaderFlags2);
            nif->readVector(mShaderFlags1Hashes, numShaderFlags1);
            nif->readVector(mShaderFlags2Hashes, numShaderFlags2);
        }

        nif->read(mUVOffset);
        nif->read(mUVScale);
    }

    void BSShaderLightingProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);

        if (nif->getUserVersion() <= 11)
            nif->read(mClamp);
    }

    void BSShaderPPLightingProperty::read(NIFStream* nif)
    {
        BSShaderLightingProperty::read(nif);

        mTextureSet.read(nif);
        if (nif->getUserVersion() == 11)
        {
            if (nif->getBethVersion() >= 15)
                mRefraction.read(nif);
            if (nif->getBethVersion() >= 25)
                mParallax.read(nif);
        }
        else if (nif->getUserVersion() >= 12)
            nif->read(mEmissiveColor);
    }

    void BSShaderPPLightingProperty::post(Reader& nif)
    {
        BSShaderLightingProperty::post(nif);

        mTextureSet.post(nif);
    }

    void BSShaderNoLightingProperty::read(NIFStream* nif)
    {
        BSShaderLightingProperty::read(nif);

        mFilename = nif->getSizedString();
        if (nif->getBethVersion() >= 27)
            nif->read(mFalloffParams);
    }

    void SkyShaderProperty::read(NIFStream* nif)
    {
        BSShaderLightingProperty::read(nif);

        mFilename = nif->getSizedString();
        mSkyObjectType = static_cast<SkyObjectType>(nif->get<uint32_t>());
    }

    void TallGrassShaderProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);

        mFilename = nif->getSizedString();
    }

    void TileShaderProperty::read(NIFStream* nif)
    {
        BSShaderLightingProperty::read(nif);

        mFilename = nif->getSizedString();
    }

    void BSSPLuminanceParams::read(NIFStream* nif)
    {
        nif->read(mLumEmittance);
        nif->read(mExposureOffset);
        nif->read(mFinalExposureMin);
        nif->read(mFinalExposureMax);
    }

    void BSSPWetnessParams::read(NIFStream* nif)
    {
        nif->read(mSpecScale);
        nif->read(mSpecPower);
        nif->read(mMinVar);
        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_FO4)
            nif->read(mEnvMapScale);
        nif->read(mFresnelPower);
        nif->read(mMetalness);
        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO4)
            nif->skip(4); // Unknown
        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            nif->skip(4); // Unknown
    }

    void BSSPMLParallaxParams::read(NIFStream* nif)
    {
        nif->read(mInnerLayerThickness);
        nif->read(mRefractionScale);
        nif->read(mInnerLayerTextureScale);
        nif->read(mEnvMapScale);
    }

    void BSSPTranslucencyParams::read(NIFStream* nif)
    {
        nif->read(mSubsurfaceColor);
        nif->read(mTransmissiveScale);
        nif->read(mTurbulence);
        nif->read(mThickObject);
        nif->read(mMixAlbedo);
    }

    void BSLightingShaderProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);

        if (!mName.empty() && nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            return;

        mTextureSet.read(nif);
        nif->read(mEmissive);
        nif->read(mEmissiveMult);

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
            nif->read(mRootMaterial);

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
            nif->skip(4); // Unknown float

        nif->read(mClamp);
        nif->read(mAlpha);
        nif->read(mRefractionStrength);

        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            nif->read(mGlossiness);
        else
            nif->read(mSmoothness);

        nif->read(mSpecular);
        nif->read(mSpecStrength);

        if (nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            nif->readArray(mLightingEffects);
        else if (nif->getBethVersion() <= 139)
        {
            nif->read(mSubsurfaceRolloff);
            nif->read(mRimlightPower);
            if (mRimlightPower == std::numeric_limits<float>::max())
                nif->read(mBacklightPower);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
        {
            nif->read(mGrayscaleToPaletteScale);
            nif->read(mFresnelPower);
            mWetness.read(nif);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
        {
            mLuminance.read(nif);
            if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_F76)
            {
                nif->read(mDoTranslucency);
                if (mDoTranslucency)
                    mTranslucency.read(nif);
                if (nif->get<uint8_t>() != 0)
                {
                    const uint32_t size = nif->get<uint32_t>();
                    mTextureArrays.reserve(size);
                    for (uint32_t i = 0; i < size; ++i)
                        nif->getSizedStrings(mTextureArrays.emplace_back(), nif->get<uint32_t>());
                }
            }
            if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
            {
                nif->skip(4); // Unknown
                nif->skip(4); // Unknown
                nif->skip(2); // Unknown
            }
        }

        switch (static_cast<BSLightingShaderType>(mType))
        {
            case BSLightingShaderType::ShaderType_EnvMap:
                if (nif->getBethVersion() <= 139)
                    nif->read(mEnvMapScale);
                if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
                {
                    nif->read(mUseSSR);
                    nif->read(mWetnessUseSSR);
                }
                break;
            case BSLightingShaderType::ShaderType_SkinTint:
                nif->read(mSkinTintColor);
                if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
                    nif->read(mSkinTintAlpha);
                break;
            case BSLightingShaderType::ShaderType_HairTint:
                nif->read(mHairTintColor);
                break;
            case BSLightingShaderType::ShaderType_ParallaxOcc:
                mParallax.read(nif);
                break;
            case BSLightingShaderType::ShaderType_MultiLayerParallax:
                mMultiLayerParallax.read(nif);
                break;
            case BSLightingShaderType::ShaderType_SparkleSnow:
                nif->read(mSparkle);
                break;
            case BSLightingShaderType::ShaderType_EyeEnvmap:
                nif->read(mCubeMapScale);
                nif->read(mLeftEyeReflectionCenter);
                nif->read(mRightEyeReflectionCenter);
                break;
            default:
                break;
        }
    }

    void BSLightingShaderProperty::post(Reader& nif)
    {
        BSShaderProperty::post(nif);

        mTextureSet.post(nif);
    }

    void BSEffectShaderProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);

        if (!mName.empty() && nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
            return;

        mSourceTexture = nif->getSizedString();

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
            nif->skip(4); // Unknown

        uint32_t miscParams = nif->get<uint32_t>();
        mClamp = miscParams & 0xFF;
        mLightingInfluence = (miscParams >> 8) & 0xFF;
        mEnvMapMinLOD = (miscParams >> 16) & 0xFF;
        nif->read(mFalloffParams);

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_F76)
            nif->read(mRefractionPower);

        nif->read(mBaseColor);
        nif->read(mBaseColorScale);
        nif->read(mFalloffDepth);
        mGreyscaleTexture = nif->getSizedString();

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
        {
            mEnvMapTexture = nif->getSizedString();
            mNormalTexture = nif->getSizedString();
            mEnvMaskTexture = nif->getSizedString();
            nif->read(mEnvMapScale);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_F76)
        {
            mReflectanceTexture = nif->getSizedString();
            mLightingTexture = nif->getSizedString();
            nif->read(mEmittanceColor);
            mEmitGradientTexture = nif->getSizedString();
            mLuminance.read(nif);
        }

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_STF)
        {
            nif->skip(7); // Unknown bytes
            nif->skip(6 * sizeof(float)); // Unknown floats
            nif->skip(1); // Unknown byte
        }
    }

    void BSSkyShaderProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);

        mFilename = nif->getSizedString();
        mSkyObjectType = static_cast<SkyObjectType>(nif->get<uint32_t>());
    }

    void BSWaterShaderProperty::read(NIFStream* nif)
    {
        BSShaderProperty::read(nif);

        nif->read(mFlags);
    }

    void NiAlphaProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        nif->read(mFlags);
        nif->read(mThreshold);
    }

    void NiDitherProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        nif->read(mFlags);
    }

    void NiFogProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        nif->read(mFlags);
        nif->read(mFogDepth);
        nif->read(mColour);
    }

    void NiMaterialProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
            nif->read(mFlags);
        if (nif->getBethVersion() < 26)
        {
            nif->read(mAmbient);
            nif->read(mDiffuse);
        }
        nif->read(mSpecular);
        nif->read(mEmissive);
        nif->read(mGlossiness);
        nif->read(mAlpha);
        if (nif->getBethVersion() >= 22)
            nif->read(mEmissiveMult);
    }

    void NiShadeProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
            nif->read(mFlags);
    }

    void NiSpecularProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        mEnable = nif->get<uint16_t>() & 1;
    }

    void NiStencilProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
                nif->read(mFlags);
            mEnabled = nif->get<uint8_t>() != 0;
            mTestFunction = static_cast<TestFunc>(nif->get<uint32_t>());
            nif->read(mStencilRef);
            nif->read(mStencilMask);
            mFailAction = static_cast<Action>(nif->get<uint32_t>());
            mZFailAction = static_cast<Action>(nif->get<uint32_t>());
            mPassAction = static_cast<Action>(nif->get<uint32_t>());
            mDrawMode = static_cast<DrawMode>(nif->get<uint32_t>());
        }
        else
        {
            nif->read(mFlags);
            mEnabled = mFlags & 0x1;
            mFailAction = static_cast<Action>((mFlags >> 1) & 0x7);
            mZFailAction = static_cast<Action>((mFlags >> 4) & 0x7);
            mPassAction = static_cast<Action>((mFlags >> 7) & 0x7);
            mDrawMode = static_cast<DrawMode>((mFlags >> 10) & 0x3);
            mTestFunction = static_cast<TestFunc>((mFlags >> 12) & 0x7);
            nif->read(mStencilRef);
            nif->read(mStencilMask);
        }
    }

    void NiVertexColorProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        nif->read(mFlags);
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
        {
            mVertexMode = static_cast<VertexMode>(nif->get<uint32_t>());
            mLightingMode = static_cast<LightMode>(nif->get<uint32_t>());
        }
        else
        {
            mVertexMode = static_cast<VertexMode>((mFlags >> 4) & 0x3);
            mLightingMode = static_cast<LightMode>((mFlags >> 3) & 0x1);
        }
    }

    void NiWireframeProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        mEnable = nif->get<uint16_t>() & 1;
    }

    void NiZBufferProperty::read(NIFStream* nif)
    {
        NiProperty::read(nif);

        nif->read(mFlags);
        if (nif->getVersion() >= NIFStream::generateVersion(4, 1, 0, 12)
            && nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
            nif->read(mTestFunction);
        else
            mTestFunction = (mFlags >> 2) & 0x7;
    }

}
