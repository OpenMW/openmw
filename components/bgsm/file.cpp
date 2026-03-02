#include "file.hpp"

#include <array>
#include <stdexcept>

#include "stream.hpp"

namespace Bgsm
{
    MaterialFilePtr parse(Files::IStreamPtr&& inputStream)
    {
        std::shared_ptr<Bgsm::MaterialFile> file;
        BGSMStream stream(std::move(inputStream));

        std::array<char, 4> signature;
        stream.readArray(signature);
        std::string shaderType(signature.data(), 4);
        if (shaderType == "BGEM")
        {
            file = std::make_shared<BGEMFile>();
            file->mShaderType = Bgsm::ShaderType::Effect;
        }
        else if (shaderType == "BGSM")
        {
            file = std::make_shared<BGSMFile>();
            file->mShaderType = Bgsm::ShaderType::Lighting;
        }
        else
            throw std::runtime_error("Invalid material file");

        file->read(stream);
        return file;
    }

    void MaterialFile::read(BGSMStream& stream)
    {
        stream.read(mVersion);
        stream.read(mClamp);
        stream.read(mUVOffset);
        stream.read(mUVScale);
        stream.read(mTransparency);
        stream.read(mAlphaBlend);
        stream.read(mSourceBlendMode);
        stream.read(mDestinationBlendMode);
        stream.read(mAlphaTestThreshold);
        stream.read(mAlphaTest);
        stream.read(mDepthWrite);
        stream.read(mDepthTest);
        stream.read(mSSR);
        stream.read(mWetnessControlSSR);
        stream.read(mDecal);
        stream.read(mTwoSided);
        stream.read(mDecalNoFade);
        stream.read(mNonOccluder);
        stream.read(mRefraction);
        stream.read(mRefractionFalloff);
        stream.read(mRefractionPower);
        if (mVersion < 10)
        {
            stream.read(mEnvMapEnabled);
            stream.read(mEnvMapMaskScale);
        }
        else
        {
            stream.read(mDepthBias);
        }
        stream.read(mGrayscaleToPaletteColor);
        if (mVersion >= 6)
            stream.read(mMaskWrites);
    }

    void BGSMFile::read(BGSMStream& stream)
    {
        MaterialFile::read(stream);

        stream.read(mDiffuseMap);
        stream.read(mNormalMap);
        stream.read(mSmoothSpecMap);
        stream.read(mGrayscaleMap);
        if (mVersion >= 3)
        {
            stream.read(mGlowMap);
            stream.read(mWrinkleMap);
            stream.read(mSpecularMap);
            stream.read(mLightingMap);
            stream.read(mFlowMap);
            if (mVersion >= 17)
                stream.read(mDistanceFieldAlphaMap);
        }
        else
        {
            stream.read(mEnvMap);
            stream.read(mGlowMap);
            stream.read(mInnerLayerMap);
            stream.read(mWrinkleMap);
            stream.read(mDisplacementMap);
        }
        stream.read(mEnableEditorAlphaThreshold);
        if (mVersion >= 8)
        {
            stream.read(mTranslucency);
            stream.read(mTranslucencyThickObject);
            stream.read(mTranslucencyMixAlbedoWithSubsurfaceColor);
            stream.read(mTranslucencySubsurfaceColor);
            stream.read(mTranslucencyTransmissiveScale);
            stream.read(mTranslucencyTurbulence);
        }
        else
        {
            stream.read(mRimLighting);
            stream.read(mRimPower);
            stream.read(mBackLightPower);
            stream.read(mSubsurfaceLighting);
            stream.read(mSubsurfaceLightingRolloff);
        }
        stream.read(mSpecularEnabled);
        stream.read(mSpecularColor);
        stream.read(mSpecularMult);
        stream.read(mSmoothness);
        stream.read(mFresnelPower);
        stream.read(mWetnessControlSpecScale);
        stream.read(mWetnessControlSpecPowerScale);
        stream.read(mWetnessControlSpecMinvar);
        if (mVersion < 10)
            stream.read(mWetnessControlEnvMapScale);
        stream.read(mWetnessControlFresnelPower);
        stream.read(mWetnessControlMetalness);
        if (mVersion >= 3)
        {
            stream.read(mPBR);
            if (mVersion >= 9)
            {
                stream.read(mCustomPorosity);
                stream.read(mPorosityValue);
            }
        }
        stream.read(mRootMaterialPath);
        stream.read(mAnisoLighting);
        stream.read(mEmitEnabled);
        if (mEmitEnabled)
            stream.read(mEmittanceColor);
        stream.read(mEmittanceMult);
        stream.read(mModelSpaceNormals);
        stream.read(mExternalEmittance);
        if (mVersion >= 12)
        {
            stream.read(mLumEmittance);
            if (mVersion >= 13)
            {
                stream.read(mUseAdaptiveEmissive);
                stream.read(mAdaptiveEmissiveExposureParams);
            }
        }
        else if (mVersion < 8)
        {
            stream.read(mBackLighting);
        }
        stream.read(mReceiveShadows);
        stream.read(mHideSecret);
        stream.read(mCastShadows);
        stream.read(mDissolveFade);
        stream.read(mAssumeShadowmask);
        stream.read(mGlowMapEnabled);
        if (mVersion < 7)
        {
            stream.read(mEnvMapWindow);
            stream.read(mEnvMapEye);
        }
        stream.read(mHair);
        stream.read(mHairTintColor);
        stream.read(mTree);
        stream.read(mFacegen);
        stream.read(mSkinTint);
        stream.read(mTessellate);
        if (mVersion < 3)
        {
            stream.read(mDisplacementMapParams);
            stream.read(mTessellationParams);
        }
        stream.read(mGrayscaleToPaletteScale);
        if (mVersion >= 1)
        {
            stream.read(mSkewSpecularAlpha);
            if (mVersion >= 3)
            {
                stream.read(mTerrain);
                if (mTerrain)
                {
                    if (mVersion == 3)
                        stream.skip(4); // Unknown

                    stream.read(mTerrainParams);
                }
            }
        }
    }

    void BGEMFile::read(BGSMStream& stream)
    {
        MaterialFile::read(stream);

        stream.read(mBaseMap);
        stream.read(mGrayscaleMap);
        stream.read(mEnvMap);
        stream.read(mNormalMap);
        stream.read(mEnvMapMask);
        if (mVersion >= 11)
        {
            stream.read(mSpecularMap);
            stream.read(mLightingMap);
            stream.read(mGlowMap);
        }
        if (mVersion >= 10)
        {
            stream.read(mEnvMapEnabled);
            stream.read(mEnvMapMaskScale);
        }
        stream.read(mBlood);
        stream.read(mEffectLighting);
        stream.read(mFalloff);
        stream.read(mFalloffColor);
        stream.read(mGrayscaleToPaletteAlpha);
        stream.read(mSoft);
        stream.read(mBaseColor);
        stream.read(mBaseColorScale);
        stream.read(mFalloffParams);
        stream.read(mLightingInfluence);
        stream.read(mEnvmapMinLOD);
        stream.read(mSoftDepth);
        if (mVersion >= 11)
            stream.read(mEmittanceColor);
        if (mVersion >= 15)
            stream.read(mAdaptiveEmissiveExposureParams);
        if (mVersion >= 16)
            stream.read(mGlowMapEnabled);
        if (mVersion >= 20)
            stream.read(mEffectPbrSpecular);
    }
}
