#include "controlled.hpp"

#include "data.hpp"

namespace Nif
{

    void NiSourceTexture::read(NIFStream *nif)
    {
        Named::read(nif);

        external = !!nif->getChar();
        if(external)
            filename = nif->getString();
        else
        {
            nif->getChar(); // always 1
            data.read(nif);
        }

        pixel = nif->getInt();
        mipmap = nif->getInt();
        alpha = nif->getInt();

        nif->getChar(); // always 1
    }

    void NiSourceTexture::post(NIFFile *nif)
    {
        Named::post(nif);
        data.post(nif);
    }

    void NiParticleGrowFade::read(NIFStream *nif)
    {
        Controlled::read(nif);
        growTime = nif->getFloat();
        fadeTime = nif->getFloat();
    }

    void NiParticleColorModifier::read(NIFStream *nif)
    {
        Controlled::read(nif);
        data.read(nif);
    }

    void NiParticleColorModifier::post(NIFFile *nif)
    {
        Controlled::post(nif);
        data.post(nif);
    }

    void NiGravity::read(NIFStream *nif)
    {
        Controlled::read(nif);

        mDecay = nif->getFloat();
        mForce = nif->getFloat();
        mType = nif->getUInt();
        mPosition = nif->getVector3();
        mDirection = nif->getVector3();
    }

    void NiPlanarCollider::read(NIFStream *nif)
    {
        Controlled::read(nif);

        mBounceFactor = nif->getFloat();
        /*unknown*/nif->getFloat();

        for (int i=0;i<10;++i)
            /*unknown*/nif->getFloat();

        mPlaneNormal = nif->getVector3();
        mPlaneDistance = nif->getFloat();
    }

    void NiParticleRotation::read(NIFStream *nif)
    {
        Controlled::read(nif);

        /*
           byte (0 or 1)
           float (1)
           float*3
        */
        nif->skip(17);
    }





}
