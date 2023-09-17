#ifndef OPENMW_COMPONENTS_NIF_PROPERTY_HPP
#define OPENMW_COMPONENTS_NIF_PROPERTY_HPP

#include "base.hpp"

namespace Nif
{

    struct Property : public NiObjectNET
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

    struct NiTexturingProperty : public Property
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

    struct NiFogProperty : public Property
    {
        unsigned short mFlags;
        float mFogDepth;
        osg::Vec3f mColour;

        void read(NIFStream* nif) override;
    };

    struct NiShadeProperty : public Property
    {
        uint16_t mFlags{ 0u };
        void read(NIFStream* nif) override
        {
            Property::read(nif);
            if (nif->getBethVersion() <= NIFFile::BethVersion::BETHVER_FO3)
                nif->read(mFlags);
        }
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
        BSSFlag1_Decal = 0x04000000,
    };

    struct BSSPParallaxParams
    {
        float mMaxPasses;
        float mScale;

        void read(NIFStream* nif);
    };

    struct BSSPRefractionParams
    {
        float mStrength;
        int32_t mPeriod;

        void read(NIFStream* nif);
    };

    struct BSShaderProperty : public NiShadeProperty
    {
        uint32_t mType{ 0u }, mShaderFlags1{ 0u }, mShaderFlags2{ 0u };
        float mEnvMapScale{ 0.f };

        void read(NIFStream* nif) override;

        // These flags are shared between BSShader and BSLightingShader
        // Shader-specific flag methods must be handled on per-record basis
        bool specular() const { return mShaderFlags1 & BSSFlag1_Specular; }
        bool decal() const { return mShaderFlags1 & BSSFlag1_Decal; }
    };

    struct BSShaderLightingProperty : public BSShaderProperty
    {
        unsigned int mClamp{ 0u };
        void read(NIFStream* nif) override;

        bool wrapT() const { return mClamp & 1; }
        bool wrapS() const { return mClamp & 2; }
    };

    struct BSShaderPPLightingProperty : public BSShaderLightingProperty
    {
        BSShaderTextureSetPtr mTextureSet;
        BSSPRefractionParams mRefraction;
        BSSPParallaxParams mParallax;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSShaderNoLightingProperty : public BSShaderLightingProperty
    {
        std::string mFilename;
        osg::Vec4f mFalloffParams;

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
        ShaderType_Dismemberment = 20
    };

    enum BSLightingShaderFlags1
    {
        BSLSFlag1_Falloff = 0x00000040,
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

    struct BSLightingShaderProperty : public BSShaderProperty
    {
        std::vector<uint32_t> mShaderFlags1Hashes, mShaderFlags2Hashes;
        osg::Vec2f mUVOffset, mUVScale;
        BSShaderTextureSetPtr mTextureSet;
        osg::Vec3f mEmissive;
        float mEmissiveMult;
        std::string mRootMaterial;
        uint32_t mClamp;
        float mAlpha;
        float mRefractionStrength;
        float mGlossiness{ 80.f };
        float mSmoothness{ 1.f };
        osg::Vec3f mSpecular;
        float mSpecStrength;
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

        osg::Vec4f mSkinTintColor;
        osg::Vec3f mHairTintColor;

        BSSPParallaxParams mParallax;
        BSSPMLParallaxParams mMultiLayerParallax;
        osg::Vec4f mSparkle;

        float mCubeMapScale;
        osg::Vec3f mLeftEyeReflectionCenter;
        osg::Vec3f mRightEyeReflectionCenter;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        bool doubleSided() const { return mShaderFlags2 & BSLSFlag2_DoubleSided; }
        bool treeAnim() const { return mShaderFlags2 & BSLSFlag2_TreeAnim; }
    };

    struct BSEffectShaderProperty : public BSShaderProperty
    {
        std::vector<uint32_t> mShaderFlags1Hashes, mShaderFlags2Hashes;
        osg::Vec2f mUVOffset, mUVScale;
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

        bool useFalloff() const { return mShaderFlags1 & BSLSFlag1_Falloff; }
        bool doubleSided() const { return mShaderFlags2 & BSLSFlag2_DoubleSided; }
        bool treeAnim() const { return mShaderFlags2 & BSLSFlag2_TreeAnim; }
    };

    struct NiDitherProperty : public Property
    {
        unsigned short flags;

        void read(NIFStream* nif) override
        {
            Property::read(nif);

            flags = nif->getUShort();
        }
    };

    struct NiZBufferProperty : public Property
    {
        unsigned short flags;
        unsigned int testFunction;

        void read(NIFStream* nif) override
        {
            Property::read(nif);

            flags = nif->getUShort();
            testFunction = (flags >> 2) & 0x7;
            if (nif->getVersion() >= NIFStream::generateVersion(4, 1, 0, 12)
                && nif->getVersion() <= NIFFile::NIFVersion::VER_OB)
                testFunction = nif->getUInt();
        }

        bool depthTest() const { return flags & 1; }

        bool depthWrite() const { return flags & 2; }
    };

    struct NiSpecularProperty : public Property
    {
        unsigned short flags;

        void read(NIFStream* nif) override
        {
            Property::read(nif);

            flags = nif->getUShort();
        }

        bool isEnabled() const { return flags & 1; }
    };

    struct NiWireframeProperty : public Property
    {
        unsigned short flags;

        void read(NIFStream* nif) override
        {
            Property::read(nif);

            flags = nif->getUShort();
        }

        bool isEnabled() const { return flags & 1; }
    };

    // The rest are all struct-based
    template <typename T>
    struct StructPropT : Property
    {
        T data;
        unsigned short flags;

        void read(NIFStream* nif) override
        {
            Property::read(nif);

            flags = nif->getUShort();
            data.read(nif);
        }
    };

    struct S_MaterialProperty
    {
        // The vector components are R,G,B
        osg::Vec3f ambient{ 1.f, 1.f, 1.f }, diffuse{ 1.f, 1.f, 1.f };
        osg::Vec3f specular, emissive;
        float glossiness{ 0.f }, alpha{ 0.f }, emissiveMult{ 1.f };

        void read(NIFStream* nif);
    };

    struct S_AlphaProperty
    {
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

        // Tested against when certain flags are set (see above.)
        unsigned char threshold;

        void read(NIFStream* nif);
    };

    /*
        Docs taken from:
        http://niftools.sourceforge.net/doc/nif/NiStencilProperty.html
     */
    struct S_StencilProperty
    {
        // Is stencil test enabled?
        unsigned char enabled;

        /*
            0   TEST_NEVER
            1   TEST_LESS
            2   TEST_EQUAL
            3   TEST_LESS_EQUAL
            4   TEST_GREATER
            5   TEST_NOT_EQUAL
            6   TEST_GREATER_EQUAL
            7   TEST_NEVER (though nifskope comment says TEST_ALWAYS, but ingame it is TEST_NEVER)
         */
        int compareFunc;
        unsigned stencilRef;
        unsigned stencilMask;
        /*
            Stencil test fail action, depth test fail action and depth test pass action:
            0   ACTION_KEEP
            1   ACTION_ZERO
            2   ACTION_REPLACE
            3   ACTION_INCREMENT
            4   ACTION_DECREMENT
            5   ACTION_INVERT
         */
        int failAction;
        int zFailAction;
        int zPassAction;
        /*
            Face draw mode:
            0   DRAW_CCW_OR_BOTH
            1   DRAW_CCW        [default]
            2   DRAW_CW
            3   DRAW_BOTH
         */
        int drawMode;

        void read(NIFStream* nif);
    };

    struct NiAlphaProperty : public StructPropT<S_AlphaProperty>
    {
        enum Flags
        {
            Flag_Blending = 0x0001,
            Flag_Testing = 0x0200,
            Flag_NoSorter = 0x2000,
        };

        bool useAlphaBlending() const { return flags & Flag_Blending; }
        bool useAlphaTesting() const { return flags & Flag_Testing; }
        bool noSorter() const { return flags & Flag_NoSorter; }

        int sourceBlendMode() const { return (flags >> 1) & 0xF; }
        int destinationBlendMode() const { return (flags >> 5) & 0xF; }
        int alphaTestMode() const { return (flags >> 10) & 0x7; }
    };

    struct NiVertexColorProperty : public Property
    {
        enum class VertexMode : unsigned int
        {
            VertMode_SrcIgnore = 0,
            VertMode_SrcEmissive = 1,
            VertMode_SrcAmbDif = 2
        };

        enum class LightMode : unsigned int
        {
            LightMode_Emissive = 0,
            LightMode_EmiAmbDif = 1
        };

        unsigned short mFlags;
        VertexMode mVertexMode;
        LightMode mLightingMode;

        void read(NIFStream* nif) override;
    };

    struct NiStencilProperty : public Property
    {
        S_StencilProperty data;
        unsigned short flags{ 0u };

        void read(NIFStream* nif) override
        {
            Property::read(nif);
            if (nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
                flags = nif->getUShort();
            data.read(nif);
        }
    };

    struct NiMaterialProperty : public Property
    {
        S_MaterialProperty data;
        unsigned short flags{ 0u };

        void read(NIFStream* nif) override
        {
            Property::read(nif);
            if (nif->getVersion() >= NIFStream::generateVersion(3, 0, 0, 0)
                && nif->getVersion() <= NIFFile::NIFVersion::VER_OB_OLD)
                flags = nif->getUShort();
            data.read(nif);
        }
    };

}
#endif
