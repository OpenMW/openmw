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

        // Renderer hints, typically of no use for us
        /* bool mIsStatic = */nif->getChar();
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,103))
            /* bool mDirectRendering = */nif->getBoolean();
        if (nif->getVersion() >= NIFStream::generateVersion(20,2,0,4))
            /* bool mPersistRenderData = */nif->getBoolean();
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
        if (nif->getVersion() >= NIFStream::generateVersion(4,2,0,2))
        {
            // Unused in NifSkope. Need to figure out what these do.
            /*bool mSpawnOnCollision = */nif->getBoolean();
            /*bool mDieOnCollision = */nif->getBoolean();
        }
    }

    void NiPlanarCollider::read(NIFStream *nif)
    {
        NiParticleCollider::read(nif);

        mExtents = nif->getVector2();
        mPosition = nif->getVector3();
        mXVector = nif->getVector3();
        mYVector = nif->getVector3();
        mPlaneNormal = nif->getVector3();
        mPlaneDistance = nif->getFloat();
    }

    void NiParticleRotation::read(NIFStream *nif)
    {
        NiParticleModifier::read(nif);

        /* bool mRandomInitialAxis = */nif->getChar();
        /* osg::Vec3f mInitialAxis = */nif->getVector3();
        /* float mRotationSpeed = */nif->getFloat();
    }

    void NiSphericalCollider::read(NIFStream* nif)
    {
        NiParticleCollider::read(nif);

        mRadius = nif->getFloat();
        mCenter = nif->getVector3();
    }

}
