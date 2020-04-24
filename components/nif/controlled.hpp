/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (controlled.h) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_NIF_CONTROLLED_HPP
#define OPENMW_COMPONENTS_NIF_CONTROLLED_HPP

#include "base.hpp"

namespace Nif
{

class NiSourceTexture : public Named
{
public:
    // Is this an external (references a separate texture file) or
    // internal (data is inside the nif itself) texture?
    bool external;

    std::string filename; // In case of external textures
    NiPixelDataPtr data;  // In case of internal textures

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

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

struct NiParticleModifier : public Record
{
    NiParticleModifierPtr next;
    ControllerPtr controller;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiParticleGrowFade : public NiParticleModifier
{
public:
    float growTime;
    float fadeTime;

    void read(NIFStream *nif);
};

class NiParticleColorModifier : public NiParticleModifier
{
public:
    NiColorDataPtr data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiGravity : public NiParticleModifier
{
public:
    float mForce;
    /* 0 - Wind (fixed direction)
     * 1 - Point (fixed origin)
     */
    int mType;
    float mDecay;
    osg::Vec3f mPosition;
    osg::Vec3f mDirection;

    void read(NIFStream *nif);
};

struct NiParticleCollider : public NiParticleModifier
{
    float mBounceFactor;
    void read(NIFStream *nif);
};

// NiPinaColada
class NiPlanarCollider : public NiParticleCollider
{
public:
    void read(NIFStream *nif);

    osg::Vec3f mPlaneNormal;
    float mPlaneDistance;
};

class NiSphericalCollider : public NiParticleCollider
{
public:
    float mRadius;
    osg::Vec3f mCenter;

    void read(NIFStream *nif);
};

class NiParticleRotation : public NiParticleModifier
{
public:
    void read(NIFStream *nif);
};



} // Namespace
#endif
