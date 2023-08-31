#include "effect.hpp"

#include "node.hpp"
#include "texture.hpp"

namespace Nif
{

    void NiDynamicEffect::read(NIFStream* nif)
    {
        Node::read(nif);
        if (nif->getVersion() >= nif->generateVersion(10, 1, 0, 106)
            && nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            nif->getBoolean(); // Switch state
        if (nif->getVersion() <= NIFFile::VER_MW
            || (nif->getVersion() >= nif->generateVersion(10, 1, 0, 0)
                && nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4))
        {
            size_t numAffectedNodes = nif->get<uint32_t>();
            nif->skip(numAffectedNodes * 4);
        }
    }

    void NiLight::read(NIFStream* nif)
    {
        NiDynamicEffect::read(nif);

        dimmer = nif->getFloat();
        ambient = nif->getVector3();
        diffuse = nif->getVector3();
        specular = nif->getVector3();
    }

    void NiTextureEffect::read(NIFStream* nif)
    {
        NiDynamicEffect::read(nif);

        // Model Projection Matrix
        nif->skip(3 * 3 * sizeof(float));

        // Model Projection Transform
        nif->skip(3 * sizeof(float));

        // Texture Filtering
        nif->skip(4);

        // Max anisotropy samples
        if (nif->getVersion() >= NIFStream::generateVersion(20, 5, 0, 4))
            nif->skip(2);

        clamp = nif->getUInt();

        textureType = (TextureType)nif->getUInt();

        coordGenType = (CoordGenType)nif->getUInt();

        texture.read(nif);

        nif->skip(1); // Use clipping plane
        nif->skip(16); // Clipping plane dimensions vector
        if (nif->getVersion() <= NIFStream::generateVersion(10, 2, 0, 0))
            nif->skip(4); // PS2-specific shorts
        if (nif->getVersion() <= NIFStream::generateVersion(4, 1, 0, 12))
            nif->skip(2); // Unknown short
    }

    void NiTextureEffect::post(Reader& nif)
    {
        NiDynamicEffect::post(nif);
        texture.post(nif);
    }

    void NiPointLight::read(NIFStream* nif)
    {
        NiLight::read(nif);

        constantAttenuation = nif->getFloat();
        linearAttenuation = nif->getFloat();
        quadraticAttenuation = nif->getFloat();
    }

    void NiSpotLight::read(NIFStream* nif)
    {
        NiPointLight::read(nif);

        cutoff = nif->getFloat();
        exponent = nif->getFloat();
    }

}
