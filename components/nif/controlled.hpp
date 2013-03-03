/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

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
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_CONTROLLED_HPP
#define OPENMW_COMPONENTS_NIF_CONTROLLED_HPP

#include "extra.hpp"
#include "controller.hpp"

namespace Nif
{

/// Anything that has a controller
class Controlled : public Extra
{
public:
    ControllerPtr controller;

    void read(NIFStream *nif)
    {
        Extra::read(nif);
        controller.read(nif);
    }

    void post(NIFFile *nif)
    {
        Extra::post(nif);
        controller.post(nif);
    }
};

/// Has name, extra-data and controller
class Named : public Controlled
{
public:
    std::string name;

    void read(NIFStream *nif)
    {
        name = nif->getString();
        Controlled::read(nif);
    }
};
typedef Named NiSequenceStreamHelper;

class NiParticleGrowFade : public Controlled
{
public:
    void read(NIFStream *nif)
    {
        Controlled::read(nif);

        // Two floats.
        nif->skip(8);
    }
};

class NiParticleColorModifier : public Controlled
{
public:
    NiColorDataPtr data;

    void read(NIFStream *nif)
    {
        Controlled::read(nif);
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controlled::post(nif);
        data.post(nif);
    }
};

class NiGravity : public Controlled
{
public:
    void read(NIFStream *nif)
    {
        Controlled::read(nif);

        // two floats, one int, six floats
        nif->skip(9*4);
    }
};

// NiPinaColada
class NiPlanarCollider : public Controlled
{
public:
    void read(NIFStream *nif)
    {
        Controlled::read(nif);

        // (I think) 4 floats + 4 vectors
        nif->skip(4*16);
    }
};

class NiParticleRotation : public Controlled
{
public:
    void read(NIFStream *nif)
    {
        Controlled::read(nif);

        /*
           byte (0 or 1)
           float (1)
           float*3
        */
        nif->skip(17);
    }
};

} // Namespace
#endif
