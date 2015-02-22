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
        osg::Vec3f ambient;
        osg::Vec3f diffuse;
        osg::Vec3f specular;

        void read(NIFStream *nif);
    };
    SLight light;

    void read(NIFStream *nif);
};

struct NiTextureEffect : Effect
{
    NiSourceTexturePtr texture;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

} // Namespace
#endif
