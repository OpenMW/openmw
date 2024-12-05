#ifndef OPENMW_COMPONENTS_NIF_TEXTURE_HPP
#define OPENMW_COMPONENTS_NIF_TEXTURE_HPP

#include "base.hpp"

namespace Nif
{

    struct NiTexture : public NiObjectNET
    {
    };

    struct NiSourceTexture : public NiTexture
    {
        enum class PixelLayout : uint32_t
        {
            Palette = 0,
            HighColor = 1,
            TrueColor = 2,
            Compressed = 3,
            BumpMap = 4,
            Default = 5,
        };

        enum class MipMapFormat : uint32_t
        {
            No = 0,
            Yes = 1,
            Default = 2,
        };

        enum class AlphaFormat : uint32_t
        {
            None = 0,
            Binary = 1,
            Smooth = 2,
            Default = 3,
        };

        struct FormatPrefs
        {
            PixelLayout mPixelLayout;
            MipMapFormat mUseMipMaps;
            AlphaFormat mAlphaFormat;
        };

        char mExternal; // References external file

        std::string mFile;
        NiPixelDataPtr mData;

        FormatPrefs mPrefs;

        char mIsStatic{ 1 };
        bool mDirectRendering{ true };
        bool mPersistRenderData{ false };

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSShaderTextureSet : public Record
    {
        enum class TextureType : uint32_t
        {
            Base = 0,
            Normal = 1,
            Glow = 2,
            Parallax = 3,
            Environment = 4,
            EnvironmentMask = 5,
            Subsurface = 6,
            BackLighting = 7,
        };
        std::vector<std::string> mTextures;

        void read(NIFStream* nif) override;
    };

}
#endif
