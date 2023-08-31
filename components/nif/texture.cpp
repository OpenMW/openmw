#include "texture.hpp"

#include "data.hpp"

namespace Nif
{

    void NiSourceTexture::read(NIFStream* nif)
    {
        Named::read(nif);

        external = nif->getChar() != 0;
        bool internal = false;
        if (external)
            filename = nif->getString();
        else
        {
            if (nif->getVersion() <= NIFStream::generateVersion(10, 0, 1, 3))
                internal = nif->getChar();
            if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
                filename = nif->getString(); // Original file path of the internal texture
        }
        if (nif->getVersion() <= NIFStream::generateVersion(10, 0, 1, 3))
        {
            if (!external && internal)
                data.read(nif);
        }
        else
        {
            data.read(nif);
        }

        pixel = nif->getUInt();
        mipmap = nif->getUInt();
        alpha = nif->getUInt();

        // Renderer hints, typically of no use for us
        /* bool mIsStatic = */ nif->getChar();
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 103))
            /* bool mDirectRendering = */ nif->getBoolean();
        if (nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 4))
            /* bool mPersistRenderData = */ nif->getBoolean();
    }

    void NiSourceTexture::post(Reader& nif)
    {
        Named::post(nif);
        data.post(nif);
    }

    void BSShaderTextureSet::read(NIFStream* nif)
    {
        nif->getSizedStrings(textures, nif->getUInt());
    }

}
