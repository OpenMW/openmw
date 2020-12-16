#include "controlled.hpp"

#include "data.hpp"

namespace Nif
{

    void NiSourceTexture::read(NIFStream *nif)
    {
        Named::read(nif);

        external = nif->getChar() != 0;
        bool internal = false;
        if (external)
            filename = nif->getString();
        else
        {
            if (nif->getVersion() <= NIFStream::generateVersion(10,0,1,3))
                internal = nif->getChar();
            if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
                filename = nif->getString(); // Original file path of the internal texture
        }
        if (nif->getVersion() <= NIFStream::generateVersion(10,0,1,3))
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

        nif->getChar(); // always 1
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,103))
            nif->getBoolean(); // Direct rendering
        if (nif->getVersion() >= NIFStream::generateVersion(20,2,0,4))
            nif->getBoolean(); // NiPersistentSrcTextureRendererData is used instead of NiPixelData
    }

    void NiSourceTexture::post(NIFFile *nif)
    {
        Named::post(nif);
        data.post(nif);
    }

    void BSShaderTextureSet::read(NIFStream *nif)
    {
        nif->getSizedStrings(textures, nif->getUInt());
    }

    void NiParticleModifier::read(NIFStream *nif)
    {
        next.read(nif);
        controller.read(nif);
    }

    void NiParticleModifier::post(NIFFile *nif)
    {
        next.post(nif);
        controller.post(nif);
    }

    void NiParticleGrowFade::read(NIFStream *nif)
    {
        NiParticleModifier::read(nif);
        growTime = nif->getFloat();
        fadeTime = nif->getFloat();
    }

    void NiParticleColorModifier::read(NIFStream *nif)
    {
        NiParticleModifier::read(nif);
        data.read(nif);
    }

    void NiParticleColorModifier::post(NIFFile *nif)
    {
        NiParticleModifier::post(nif);
        data.post(nif);
    }

    void NiGravity::read(NIFStream *nif)
    {
        NiParticleModifier::read(nif);

        mDecay = nif->getFloat();
        mForce = nif->getFloat();
        mType = nif->getUInt();
        mPosition = nif->getVector3();
        mDirection = nif->getVector3();
    }

    void NiParticleCollider::read(NIFStream *nif)
    {
        NiParticleModifier::read(nif);

        mBounceFactor = nif->getFloat();
        if (nif->getVersion() >= NIFStream::generateVersion(4,2,2,0))
        {
            // Unused in NifSkope. Need to figure out what these do.
            /*bool spawnOnCollision = */nif->getBoolean();
            /*bool dieOnCollision = */nif->getBoolean();
        }
    }

    void NiPlanarCollider::read(NIFStream *nif)
    {
        NiParticleCollider::read(nif);

        /*unknown*/nif->getFloat();

        for (int i=0;i<10;++i)
            /*unknown*/nif->getFloat();

        mPlaneNormal = nif->getVector3();
        mPlaneDistance = nif->getFloat();
    }

    void NiParticleRotation::read(NIFStream *nif)
    {
        NiParticleModifier::read(nif);

        /*
           byte (0 or 1)
           float (1)
           float*3
        */
        nif->skip(17);
    }

    void NiSphericalCollider::read(NIFStream* nif)
    {
        NiParticleCollider::read(nif);

        mRadius = nif->getFloat();
        mCenter = nif->getVector3();
    }

}
