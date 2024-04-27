#ifndef OPENMW_COMPONENTS_NIF_PROPERTY_HPP
#define OPENMW_COMPONENTS_NIF_PROPERTY_HPP

#include "base.hpp"

namespace Nif
{

    struct NiProperty : NiObjectNET
    {
    };

    struct NiTextureTransform
    {
        enum class Method : uint32_t
        {
            // Back = inverse of mOrigin.
            // FromMaya = inverse of the V axis with a positive translation along V of 1 unit.
            MayaLegacy = 0, // mOrigin * mRotation * Back * mOffset * mScale
            Max = 1, // mOrigin * mScale * mRotation * mOffset * Back
            Maya = 2, // mOrigin * mRotation * Back * FromMaya * mOffset * mScale
        };

        osg::Vec2f mOffset;
        osg::Vec2f mScale;
        float mRotation;
        Method mTransformMethod;
        osg::Vec2f mOrigin;

        void read(NIFStream* nif);
    };

    struct NiTexturingProperty : NiProperty
    {
        enum class ApplyMode : uint32_t
        {
            Replace = 0,
            Decal = 1,
            Modulate = 2,
            Hilight = 3, // PS2-specific?
            Hilight2 = 4, // Used for Oblivion parallax
        };

        enum TextureType
        {
            BaseTexture = 0,
            DarkTexture = 1,
            DetailTexture = 2,
            GlossTexture = 3,
            GlowTexture = 4,
            BumpTexture = 5,
            DecalTexture = 6,
        };

        // A sub-texture
        struct Texture
        {
            bool mEnabled;
            NiSourceTexturePtr mSourceTexture;
            uint32_t mClamp;
            uint32_t mFilter;
            uint16_t mMaxAnisotropy;
            uint32_t mUVSet;
            bool mHasTransform;
            NiTextureTransform mTransform;

            void read(NIFStream* nif);
            void post(Reader& nif);

            bool wrapT() const { return mClamp & 1; }
            bool wrapS() const { return mClamp & 2; }
        };

        uint16_t mFlags{ 0u };
        ApplyMode mApplyMode{ ApplyMode::Modulate };

        std::vector<Texture> mTextures;
        std::vector<Texture> mShaderTextures;
        std::vector<uint32_t> mShaderIds;

        osg::Vec2f mEnvMapLumaBias;
        osg::Vec4f mBumpMapMatrix;
        float mParallaxOffset;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiShadeProperty : NiProperty
    {
        uint16_t mFlags{ 0u };

        void read(NIFStream* nif) override;
    };

    enum class BSShaderType : uint32_t
    {
        ShaderType_TallGrass = 0,
        ShaderType_Default = 1,
        ShaderType_Sky = 10,
        ShaderType_Skin = 14,
        ShaderType_Water = 17,
        ShaderType_Lighting30 = 29,
        ShaderType_Tile = 32,
        ShaderType_NoLighting = 33
    };

    enum BSShaderFlags1
    {
        BSSFlag1_Specular = 0x00000001,
        BSSFlag1_Refraction = 0x00008000,
        BSSFlag1_FireRefraction = 0x00010000,
        BSSFlag1_Decal = 0x04000000,
        BSSFlag1_DepthTest = 0x80000000,
    };

    enum BSShaderFlags2
    {
        BSSFlag2_DepthWrite = 0x00000001,
    };

    struct BSSPParallaxParams
    {
        float mMaxPasses{ 4.f };
        float mScale{ 1.f };

        void read(NIFStream* nif);
    };

    struct BSSPRefractionParams
    {
        float mStrength{ 0.f };
        int32_t mPeriod{ 0 };

        void read(NIFStream* nif);
    };

    struct BSShaderProperty : NiShadeProperty
    {
        uint32_t mType{ 0u }, mShaderFlags1{ 0u }, mShaderFlags2{ 0u };
        float mEnvMapScale{ 0.f };
        std::vector<uint32_t> mShaderFlags1Hashes, mShaderFlags2Hashes;
        osg::Vec2f mUVOffset, mUVScale;

        void read(NIFStream* nif) override;

        // These flags are shared between BSShader and BSLightingShader
        // Shader-specific flag methods must be handled on per-record basis
        bool specular() const { return mShaderFlags1 & BSSFlag1_Specular; }
        bool decal() const { return mShaderFlags1 & BSSFlag1_Decal; }
        bool depthTest() const { return mShaderFlags1 & BSSFlag1_DepthTest; }
        bool depthWrite() const { return mShaderFlags2 & BSSFlag2_DepthWrite; }
        bool refraction() const { return mShaderFlags1 & BSSFlag1_Refraction; }
        bool fireRefraction() const { return mShaderFlags1 & BSSFlag1_FireRefraction; }
    };

    struct BSShaderLightingProperty : BSShaderProperty
    {
        uint32_t mClamp{ 3 };

        void read(NIFStream* nif) override;

        bool wrapT() const { return mClamp & 1; }
        bool wrapS() const { return mClamp & 2; }
    };

    struct BSShaderPPLightingProperty : BSShaderLightingProperty
    {
        BSShaderTextureSetPtr mTextureSet;
        BSSPRefractionParams mRefraction;
        BSSPParallaxParams mParallax;
        osg::Vec4f mEmissiveColor;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSShaderNoLightingProperty : BSShaderLightingProperty
    {
        std::string mFilename;
        osg::Vec4f mFalloffParams;

        void read(NIFStream* nif) override;
    };

    enum class SkyObjectType : uint32_t
    {
        SkyTexture = 0,
        SkySunglare = 1,
        Sky = 2,
        SkyClouds = 3,
        SkyStars = 5,
        SkyMoonStarsMask = 7,
    };

    struct SkyShaderProperty : BSShaderLightingProperty
    {
        std::string mFilename;
        SkyObjectType mSkyObjectType;

        void read(NIFStream* nif) override;
    };

    struct TallGrassShaderProperty : BSShaderProperty
    {
        std::string mFilename;

        void read(NIFStream* nif) override;
    };

    struct TileShaderProperty : BSShaderLightingProperty
    {
        std::string mFilename;

        void read(NIFStream* nif) override;
    };

    enum class BSLightingShaderType : uint32_t
    {
        ShaderType_Default = 0,
        ShaderType_EnvMap = 1,
        ShaderType_Glow = 2,
        ShaderType_Parallax = 3,
        ShaderType_FaceTint = 4,
        ShaderType_SkinTint = 5,
        ShaderType_HairTint = 6,
        ShaderType_ParallaxOcc = 7,
        ShaderType_MultitexLand = 8,
        ShaderType_LODLand = 9,
        ShaderType_Snow = 10,
        ShaderType_MultiLayerParallax = 11,
        ShaderType_TreeAnim = 12,
        ShaderType_LODObjects = 13,
        ShaderType_SparkleSnow = 14,
        ShaderType_LODObjectsHD = 15,
        ShaderType_EyeEnvmap = 16,
        ShaderType_Cloud = 17,
        ShaderType_LODNoise = 18,
        ShaderType_MultitexLandLODBlend = 19,
        ShaderType_Dismemberment = 20,
        ShaderType_Terrain = 21, // FO76+, technically 17
    };

    enum BSLightingShaderFlags1
    {
        BSLSFlag1_Falloff = 0x00000040,
        BSLSFlag1_SoftEffect = 0x40000000,
    };

    enum BSLightingShaderFlags2
    {
        BSLSFlag2_DoubleSided = 0x00000010,
        BSLSFlag2_TreeAnim = 0x20000000,
    };

    struct BSSPLuminanceParams
    {
        float mLumEmittance;
        float mExposureOffset;
        float mFinalExposureMin, mFinalExposureMax;

        void read(NIFStream* nif);
    };

    struct BSSPWetnessParams
    {
        float mSpecScale;
        float mSpecPower;
        float mMinVar;
        float mEnvMapScale;
        float mFresnelPower;
        float mMetalness;

        void read(NIFStream* nif);
    };

    struct BSSPMLParallaxParams
    {
        float mInnerLayerThickness;
        float mRefractionScale;
        osg::Vec2f mInnerLayerTextureScale;
        float mEnvMapScale;

        void read(NIFStream* nif);
    };

    struct BSSPTranslucencyParams
    {
        osg::Vec3f mSubsurfaceColor;
        float mTransmissiveScale;
        float mTurbulence;
        bool mThickObject;
        bool mMixAlbedo;

        void read(NIFStream* nif);
    };

    struct BSLightingShaderProperty : BSShaderProperty
    {
        BSShaderTextureSetPtr mTextureSet;
        osg::Vec3f mEmissive;
        float mEmissiveMult{ 1.f };
        std::string mRootMaterial;
        uint32_t mClamp{ 3 };
        float mAlpha{ 1.f };
        float mRefractionStrength;
        float mGlossiness{ 80.f };
        float mSmoothness{ 1.f };
        osg::Vec3f mSpecular;
        float mSpecStrength{ 1.f };
        std::array<float, 2> mLightingEffects;
        float mSubsurfaceRolloff;
        float mRimlightPower;
        float mBacklightPower;
        float mGrayscaleToPaletteScale{ 1.f };
        float mFresnelPower{ 5.f };
        BSSPWetnessParams mWetness;
        bool mDoTranslucency{ false };
        BSSPTranslucencyParams mTranslucency;
        std::vector<std::vector<std::string>> mTextureArrays;
        BSSPLuminanceParams mLuminance;

        bool mUseSSR;
        bool mWetnessUseSSR;

        osg::Vec3f mSkinTintColor;
        float mSkinTintAlpha{ 1.f };
        osg::Vec3f mHairTintColor;

        BSSPParallaxParams mParallax;
        BSSPMLParallaxParams mMultiLayerParallax;
        osg::Vec4f mSparkle;

        float mCubeMapScale;
        osg::Vec3f mLeftEyeReflectionCenter;
        osg::Vec3f mRightEyeReflectionCenter;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        bool wrapT() const { return mClamp & 1; }
        bool wrapS() const { return mClamp & 2; }

        bool doubleSided() const { return mShaderFlags2 & BSLSFlag2_DoubleSided; }
        bool treeAnim() const { return mShaderFlags2 & BSLSFlag2_TreeAnim; }
    };

    struct BSEffectShaderProperty : BSShaderProperty
    {
        std::string mSourceTexture;
        uint8_t mClamp;
        uint8_t mLightingInfluence;
        uint8_t mEnvMapMinLOD;
        osg::Vec4f mFalloffParams;
        float mRefractionPower;
        osg::Vec4f mBaseColor;
        float mBaseColorScale;
        float mFalloffDepth;
        std::string mGreyscaleTexture;
        std::string mEnvMapTexture;
        std::string mNormalTexture;
        std::string mEnvMaskTexture;
        float mEnvMapScale;
        std::string mReflectanceTexture;
        std::string mLightingTexture;
        osg::Vec3f mEmittanceColor;
        std::string mEmitGradientTexture;
        BSSPLuminanceParams mLuminance;

        void read(NIFStream* nif) override;

        bool wrapT() const { return mClamp & 1; }
        bool wrapS() const { return mClamp & 2; }

        bool useFalloff() const { return mShaderFlags1 & BSLSFlag1_Falloff; }
        bool softEffect() const { return mShaderFlags1 & BSLSFlag1_SoftEffect; }
        bool doubleSided() const { return mShaderFlags2 & BSLSFlag2_DoubleSided; }
        bool treeAnim() const { return mShaderFlags2 & BSLSFlag2_TreeAnim; }
    };

    struct BSSkyShaderProperty : BSShaderProperty
    {
        std::string mFilename;
        SkyObjectType mSkyObjectType;

        void read(NIFStream* nif) override;
    };

    struct BSWaterShaderProperty : BSShaderProperty
    {
        enum Flags
        {
            Flag_Displacement = 0x0001,
            Flag_LOD = 0x0002,
            Flag_Depth = 0x0004,
            Flag_ActorInWater = 0x0008,
            Flag_ActorInWaterIsMoving = 0x0010,
            Flag_Underwater = 0x0020,
            Flag_Reflections = 0x0040,
            Flag_Refractions = 0x0080,
            Flag_VertexUV = 0x0100,
            Flag_VertexAlphaDepth = 0x0200,
            Flag_Procedural = 0x0400,
            Flag_Fog = 0x0800,
            Flag_UpdateConstants = 0x1000,
            Flag_CubeMap = 0x2000,
        };

        uint32_t mFlags;

        void read(NIFStream* nif) override;
    };

    struct NiAlphaProperty : NiProperty
    {
        enum Flags
        {
            Flag_Blending = 0x0001,
            Flag_Testing = 0x0200,
            Flag_NoSorter = 0x2000,
        };

        uint16_t mFlags;
        uint8_t mThreshold;

        void read(NIFStream* nif) override;

        bool useAlphaBlending() const { return mFlags & Flag_Blending; }
        bool useAlphaTesting() const { return mFlags & Flag_Testing; }
        bool noSorter() const { return mFlags & Flag_NoSorter; }

        /*
            NiAlphaProperty blend modes (glBlendFunc):
            0000 GL_ONE
            0001 GL_ZERO
            0010 GL_SRC_COLOR
            0011 GL_ONE_MINUS_SRC_COLOR
            0100 GL_DST_COLOR
            0101 GL_ONE_MINUS_DST_COLOR
            0110 GL_SRC_ALPHA
            0111 GL_ONE_MINUS_SRC_ALPHA
            1000 GL_DST_ALPHA
            1001 GL_ONE_MINUS_DST_ALPHA
            1010 GL_SRC_ALPHA_SATURATE

            test modes (glAlphaFunc):
            000 GL_ALWAYS
            001 GL_LESS
            010 GL_EQUAL
            011 GL_LEQUAL
            100 GL_GREATER
            101 GL_NOTEQUAL
            110 GL_GEQUAL
            111 GL_NEVER

            Taken from:
            http://niftools.sourceforge.net/doc/nif/NiAlphaProperty.html
        */

        int sourceBlendMode() const { return (mFlags >> 1) & 0xF; }
        int destinationBlendMode() const { return (mFlags >> 5) & 0xF; }
        int alphaTestMode() const { return (mFlags >> 10) & 0x7; }
    };

    struct NiDitherProperty : NiProperty
    {
        uint16_t mFlags;

        void read(NIFStream* nif) override;
    };

    struct NiFogProperty : NiProperty
    {
        enum Flags : uint16_t
        {
            Enabled = 0x02,
            Radial = 0x08,
            VertexAlpha = 0x10,
        };

        uint16_t mFlags;
        float mFogDepth;
        osg::Vec3f mColour;

        void read(NIFStream* nif) override;

        bool enabled() const { return mFlags & Flags::Enabled; }
        bool radial() const { return mFlags & Flags::Radial; }
        bool vertexAlpha() const { return mFlags & Flags::VertexAlpha; }
    };

    struct NiMaterialProperty : NiProperty
    {
        uint16_t mFlags{ 0u };
        osg::Vec3f mAmbient{ 1.f, 1.f, 1.f };
        osg::Vec3f mDiffuse{ 1.f, 1.f, 1.f };
        osg::Vec3f mSpecular;
        osg::Vec3f mEmissive;
        float mGlossiness{ 0.f };
        float mAlpha{ 0.f };
        float mEmissiveMult{ 1.f };

        void read(NIFStream* nif) override;
    };

    struct NiSpecularProperty : NiProperty
    {
        bool mEnable;

        void read(NIFStream* nif) override;
    };

    struct NiStencilProperty : NiProperty
    {
        enum class TestFunc : uint32_t
        {
            Never = 0,
            Less = 1,
            Equal = 2,
            LessEqual = 3,
            Greater = 4,
            NotEqual = 5,
            GreaterEqual = 6,
            Always = 7,
        };

        enum class Action : uint32_t
        {
            Keep = 0,
            Zero = 1,
            Replace = 2,
            Increment = 3,
            Decrement = 4,
            Invert = 5,
        };

        enum class DrawMode : uint32_t
        {
            Default = 0,
            CounterClockwise = 1,
            Clockwise = 2,
            Both = 3,
        };

        uint16_t mFlags{ 0u };
        bool mEnabled;
        TestFunc mTestFunction;
        uint32_t mStencilRef;
        uint32_t mStencilMask;
        Action mFailAction;
        Action mZFailAction;
        Action mPassAction;
        DrawMode mDrawMode;

        void read(NIFStream* nif) override;
    };

    struct NiVertexColorProperty : NiProperty
    {
        enum class VertexMode : uint32_t
        {
            VertMode_SrcIgnore = 0,
            VertMode_SrcEmissive = 1,
            VertMode_SrcAmbDif = 2
        };

        enum class LightMode : uint32_t
        {
            LightMode_Emissive = 0,
            LightMode_EmiAmbDif = 1
        };

        uint16_t mFlags;
        VertexMode mVertexMode;
        LightMode mLightingMode;

        void read(NIFStream* nif) override;
    };

    struct NiWireframeProperty : NiProperty
    {
        bool mEnable;

        void read(NIFStream* nif) override;
    };

    struct NiZBufferProperty : NiProperty
    {
        uint16_t mFlags;
        uint32_t mTestFunction;

        void read(NIFStream* nif) override;

        bool depthTest() const { return mFlags & 1; }
        bool depthWrite() const { return mFlags & 2; }
    };

}
#endif
