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
    int pixel;

    /* Mipmap format
        0 - no
        1 - yes
        2 - default */
    int mipmap;

    /* Alpha
        0 - none
        1 - binary
        2 - smooth
        3 - default (use material alpha, or multiply material with texture if present)
    */
    int alpha;

    void read(NIFStream *nif)
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

    void post(NIFFile *nif)
    {
        Named::post(nif);
        data.post(nif);
    }
};

class NiParticleGrowFade : public Controlled
{
public:
    float growTime;
    float fadeTime;

    void read(NIFStream *nif)
    {
        Controlled::read(nif);
        growTime = nif->getFloat();
        fadeTime = nif->getFloat();
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
    float mForce;
    /* 0 - Wind (fixed direction)
     * 1 - Point (fixed origin)
     */
    int mType;
    Ogre::Vector3 mPosition;
    Ogre::Vector3 mDirection;

    void read(NIFStream *nif)
    {
        Controlled::read(nif);

        /*unknown*/nif->getFloat();
        mForce = nif->getFloat();
        mType = nif->getUInt();
        mPosition = nif->getVector3();
        mDirection = nif->getVector3();
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
