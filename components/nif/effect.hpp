/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

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
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_EFFECT_HPP
#define OPENMW_COMPONENTS_NIF_EFFECT_HPP

#include "node.hpp"

namespace Nif
{

typedef Node Effect;

// Used for NiAmbientLight and NiDirectionalLight. Might also work for
// NiPointLight and NiSpotLight?
struct NiLight : Effect
{
    struct SLight
    {
        float dimmer;
        Ogre::Vector3 ambient;
        Ogre::Vector3 diffuse;
        Ogre::Vector3 specular;

        void read(NIFStream *nif)
        {
            dimmer = nif->getFloat();
            ambient = nif->getVector3();
            diffuse = nif->getVector3();
            specular = nif->getVector3();
        }
    };
    SLight light;

    void read(NIFStream *nif)
    {
        Effect::read(nif);

        nif->getInt(); // 1
        nif->getInt(); // 1?
        light.read(nif);
    }
};

struct NiTextureEffect : Effect
{
    NiSourceTexturePtr texture;

    void read(NIFStream *nif)
    {
        Effect::read(nif);

        int tmp = nif->getInt();
        if(tmp) nif->getInt(); // always 1?

        /*
           3 x Vector4 = [1,0,0,0]
           int = 2
           int = 0 or 3
           int = 2
           int = 2
        */
        nif->skip(16*4);

        texture.read(nif);

        /*
           byte = 0
           vector4 = [1,0,0,0]
           short = 0
           short = -75
           short = 0
        */
        nif->skip(23);
    }

    void post(NIFFile *nif)
    {
        Effect::post(nif);
        texture.post(nif);
    }
};

} // Namespace
#endif
