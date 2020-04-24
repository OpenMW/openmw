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
            internal = nif->getChar();

        if (!external && internal)
            data.read(nif);

        pixel = nif->getUInt();
        mipmap = nif->getUInt();
        alpha = nif->getUInt();

        nif->getChar(); // always 1
    }

    void NiSourceTexture::post(NIFFile *nif)
    {
        Named::post(nif);
        data.post(nif);
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
