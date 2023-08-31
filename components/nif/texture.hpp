#ifndef OPENMW_COMPONENTS_NIF_TEXTURE_HPP
#define OPENMW_COMPONENTS_NIF_TEXTURE_HPP

#include "base.hpp"

namespace Nif
{

    struct NiTexture : public Named
    {
    };

    struct NiSourceTexture : public NiTexture
    {
        // Is this an external (references a separate texture file) or
        // internal (data is inside the nif itself) texture?
        bool external;

        std::string filename; // In case of external textures
        NiPixelDataPtr data; // In case of internal textures

        /* Pixel layout
            0 - Palettised
            1 - High color 16
            2 - True color 32
            3 - Compressed
            4 - Bumpmap
            5 - Default */
        unsigned int pixel;

        /* Mipmap format
            0 - no
            1 - yes
            2 - default */
        unsigned int mipmap;

        /* Alpha
            0 - none
            1 - binary
            2 - smooth
            3 - default (use material alpha, or multiply material with texture if present)
        */
        unsigned int alpha;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSShaderTextureSet : public Record
    {
        enum TextureType
        {
            TextureType_Base = 0,
            TextureType_Normal = 1,
            TextureType_Glow = 2,
            TextureType_Parallax = 3,
            TextureType_Env = 4,
            TextureType_EnvMask = 5,
            TextureType_Subsurface = 6,
            TextureType_BackLighting = 7
        };
        std::vector<std::string> textures;

        void read(NIFStream* nif) override;
    };

}
#endif
