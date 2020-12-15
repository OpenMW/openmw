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

struct NiSourceTexture : public Named
{
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

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
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

    void read(NIFStream *nif) override;
};

struct NiParticleModifier : public Record
{
    NiParticleModifierPtr next;
    ControllerPtr controller;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiParticleGrowFade : public NiParticleModifier
{
    float growTime;
    float fadeTime;

    void read(NIFStream *nif) override;
};

struct NiParticleColorModifier : public NiParticleModifier
{
    NiColorDataPtr data;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiGravity : public NiParticleModifier
{
    float mForce;
    /* 0 - Wind (fixed direction)
     * 1 - Point (fixed origin)
     */
    int mType;
    float mDecay;
    osg::Vec3f mPosition;
    osg::Vec3f mDirection;

    void read(NIFStream *nif) override;
};

struct NiParticleCollider : public NiParticleModifier
{
    float mBounceFactor;
    void read(NIFStream *nif) override;
};

// NiPinaColada
struct NiPlanarCollider : public NiParticleCollider
{
    void read(NIFStream *nif) override;

    osg::Vec3f mPlaneNormal;
    float mPlaneDistance;
};

struct NiSphericalCollider : public NiParticleCollider
{
    float mRadius;
    osg::Vec3f mCenter;

    void read(NIFStream *nif) override;
};

struct NiParticleRotation : public NiParticleModifier
{
    void read(NIFStream *nif) override;
};



} // Namespace
#endif
