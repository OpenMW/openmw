#include "controller.hpp"

#include "node.hpp"
#include "data.hpp"

namespace Nif
{

    void Controller::read(NIFStream *nif)
    {
        next.read(nif);

        flags = nif->getUShort();

        frequency = nif->getFloat();
        phase = nif->getFloat();
        timeStart = nif->getFloat();
        timeStop = nif->getFloat();

        target.read(nif);
    }

    void Controller::post(NIFFile *nif)
    {
        Record::post(nif);
        next.post(nif);
        target.post(nif);
    }

    void NiParticleSystemController::read(NIFStream *nif)
    {
        Controller::read(nif);

        velocity = nif->getFloat();
        velocityRandom = nif->getFloat();
        verticalDir = nif->getFloat();
        verticalAngle = nif->getFloat();
        horizontalDir = nif->getFloat();
        horizontalAngle = nif->getFloat();
        /*normal?*/ nif->getVector3();
        /*color?*/ nif->getVector4();
        size = nif->getFloat();
        startTime = nif->getFloat();
        stopTime = nif->getFloat();
        nif->getChar();
        emitRate = nif->getFloat();
        lifetime = nif->getFloat();
        lifetimeRandom = nif->getFloat();

        emitFlags = nif->getUShort();
        offsetRandom = nif->getVector3();

        emitter.read(nif);

        /* Unknown Short, 0?
         * Unknown Float, 1.0?
         * Unknown Int, 1?
         * Unknown Int, 0?
         * Unknown Short, 0?
         */
        nif->skip(16);

        numParticles = nif->getUShort();
        activeCount = nif->getUShort();

        particles.resize(numParticles);
        for(size_t i = 0;i < particles.size();i++)
        {
            particles[i].velocity = nif->getVector3();
            nif->getVector3(); /* unknown */
            particles[i].lifetime = nif->getFloat();
            particles[i].lifespan = nif->getFloat();
            particles[i].timestamp = nif->getFloat();
            nif->getUShort(); /* unknown */
            particles[i].vertex = nif->getUShort();
        }

        nif->getUInt(); /* -1? */
        affectors.read(nif);
        colliders.read(nif);
        nif->getChar();
    }

    void NiParticleSystemController::post(NIFFile *nif)
    {
        Controller::post(nif);
        emitter.post(nif);
        affectors.post(nif);
        colliders.post(nif);
    }

    void NiMaterialColorController::read(NIFStream *nif)
    {
        Controller::read(nif);
        // Two bits that correspond to the controlled material color.
        // 00: Ambient
        // 01: Diffuse
        // 10: Specular
        // 11: Emissive
        targetColor = (flags >> 4) & 3;
        data.read(nif);
    }

    void NiMaterialColorController::post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }

    void NiLookAtController::read(NIFStream *nif)
    {
        Controller::read(nif);
        target.read(nif);
    }

    void NiLookAtController::post(NIFFile *nif)
    {
        Controller::post(nif);
        target.post(nif);
    }

    void NiPathController::read(NIFStream *nif)
    {
        Controller::read(nif);

        /*
           int = 1
           2xfloat
           short = 0 or 1
        */
        nif->skip(14);
        posData.read(nif);
        floatData.read(nif);
    }

    void NiPathController::post(NIFFile *nif)
    {
        Controller::post(nif);

        posData.post(nif);
        floatData.post(nif);
    }

    void NiUVController::read(NIFStream *nif)
    {
        Controller::read(nif);

        uvSet = nif->getUShort();
        data.read(nif);
    }

    void NiUVController::post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }

    void NiKeyframeController::read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void NiKeyframeController::post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }

    void NiAlphaController::read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void NiAlphaController::post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }

    void NiRollController::read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void NiRollController::post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }

    void NiGeomMorpherController::read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_MW)
            /*bool alwaysActive = */nif->getChar(); // Always 0
    }

    void NiGeomMorpherController::post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }

    void NiVisController::read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void NiVisController::post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }

    void NiFlipController::read(NIFStream *nif)
    {
        Controller::read(nif);
        mTexSlot = nif->getUInt();
        /*unknown=*/nif->getUInt();/*0?*/
        mDelta = nif->getFloat();
        mSources.read(nif);
    }

    void NiFlipController::post(NIFFile *nif)
    {
        Controller::post(nif);
        mSources.post(nif);
    }

}
