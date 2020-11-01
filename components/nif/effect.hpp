/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

  This file (effect.h) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_NIF_EFFECT_HPP
#define OPENMW_COMPONENTS_NIF_EFFECT_HPP

#include "node.hpp"

namespace Nif
{

struct NiDynamicEffect : public Node
{
    void read(NIFStream *nif) override
    {
        Node::read(nif);
        if (nif->getVersion() >= nif->generateVersion(10,1,0,106)
         && nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO4)
            nif->getBoolean(); // Switch state
        unsigned int numAffectedNodes = nif->getUInt();
        for (unsigned int i=0; i<numAffectedNodes; ++i)
            nif->getUInt(); // ref to another Node
    }
};

// Used as base for NiAmbientLight, NiDirectionalLight, NiPointLight and NiSpotLight.
struct NiLight : NiDynamicEffect
{
    float dimmer;
    osg::Vec3f ambient;
    osg::Vec3f diffuse;
    osg::Vec3f specular;

    void read(NIFStream *nif) override;
};

struct NiPointLight : public NiLight
{
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;

    void read(NIFStream *nif) override;
};

struct NiSpotLight : public NiPointLight
{
    float cutoff;
    float exponent;
    void read(NIFStream *nif) override;
};

struct NiTextureEffect : NiDynamicEffect
{
    NiSourceTexturePtr texture;
    unsigned int clamp;

    enum TextureType
    {
        Projected_Light = 0,
        Projected_Shadow = 1,
        Environment_Map = 2,
        Fog_Map = 3
    };
    TextureType textureType;

    enum CoordGenType
    {
        World_Parallel = 0,
        World_Perspective,
        Sphere_Map,
        Specular_Cube_Map,
        Diffuse_Cube_Map
    };
    CoordGenType coordGenType;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

} // Namespace
#endif
