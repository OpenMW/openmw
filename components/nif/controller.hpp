/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (controller.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  https://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_CONTROLLER_HPP
#define OPENMW_COMPONENTS_NIF_CONTROLLER_HPP

#include "base.hpp"

namespace Nif
{

class NiParticleSystemController : public Controller
{
public:
    struct Particle {
        osg::Vec3f velocity;
        float lifetime;
        float lifespan;
        float timestamp;
        int vertex;
    };

    float velocity;
    float velocityRandom;

    float verticalDir; // 0=up, pi/2=horizontal, pi=down
    float verticalAngle;
    float horizontalDir;
    float horizontalAngle;

    float size;
    float startTime;
    float stopTime;

    float emitRate;
    float lifetime;
    float lifetimeRandom;

    enum EmitFlags
    {
        NoAutoAdjust = 0x1 // If this flag is set, we use the emitRate value. Otherwise,
                           // we calculate an emit rate so that the maximum number of particles
                           // in the system (numParticles) is never exceeded.
    };
    int emitFlags;

    osg::Vec3f offsetRandom;

    NodePtr emitter;

    int numParticles;
    int activeCount;
    std::vector<Particle> particles;

    NiParticleModifierPtr affectors;
    NiParticleModifierPtr colliders;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};
using NiBSPArrayController = NiParticleSystemController;

class NiMaterialColorController : public Controller
{
public:
    NiPoint3InterpolatorPtr interpolator;
    NiPosDataPtr data;
    unsigned int targetColor;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

class NiPathController : public Controller
{
public:
    NiPosDataPtr posData;
    NiFloatDataPtr floatData;

    enum Flags
    {
        Flag_OpenCurve      = 0x020,
        Flag_AllowFlip      = 0x040,
        Flag_Bank           = 0x080,
        Flag_ConstVelocity  = 0x100,
        Flag_Follow         = 0x200,
        Flag_FlipFollowAxis = 0x400
    };

    int bankDir;
    float maxBankAngle, smoothing;
    short followAxis;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

class NiLookAtController : public Controller
{
public:
    NodePtr target;
    unsigned short lookAtFlags{0};

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

class NiUVController : public Controller
{
public:
    NiUVDataPtr data;
    unsigned int uvSet;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

class NiKeyframeController : public Controller
{
public:
    NiKeyframeDataPtr data;
    NiTransformInterpolatorPtr interpolator;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiFloatInterpController : public Controller
{
    NiFloatDataPtr data;
    NiFloatInterpolatorPtr interpolator;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

class NiAlphaController : public NiFloatInterpController { };
class NiRollController : public NiFloatInterpController { };

class NiGeomMorpherController : public Controller
{
public:
    NiMorphDataPtr data;
    NiFloatInterpolatorList interpolators;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

class NiVisController : public Controller
{
public:
    NiVisDataPtr data;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

class NiFlipController : public Controller
{
public:
    NiFloatInterpolatorPtr mInterpolator;
    int mTexSlot; // NiTexturingProperty::TextureType
    float mDelta; // Time between two flips. delta = (start_time - stop_time) / num_sources
    NiSourceTextureList mSources;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct bhkBlendController : public Controller
{
    void read(NIFStream *nif) override;
};

struct Interpolator : public Record { };

struct NiPoint3Interpolator : public Interpolator
{
    osg::Vec3f defaultVal;
    NiPosDataPtr data;
    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiBoolInterpolator : public Interpolator
{
    bool defaultVal;
    NiBoolDataPtr data;
    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiFloatInterpolator : public Interpolator
{
    float defaultVal;
    NiFloatDataPtr data;
    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiTransformInterpolator : public Interpolator
{
    osg::Vec3f defaultPos;
    osg::Quat defaultRot;
    float defaultScale;
    NiKeyframeDataPtr data;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

} // Namespace
#endif
