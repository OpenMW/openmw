/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (property.h) is part of the OpenMW package.

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

#ifndef _NIF_PROPERTY_H_
#define _NIF_PROPERTY_H_

#include "controlled.h"

namespace Nif
{

struct Property : Named
{
  // The meaning of these depends on the actual property type.
  int flags;

  void read(NIFFile *nif)
  {
    Named::read(nif);
    flags = nif->getUshort();
  }
};

struct NiTexturingProperty : Property
{
  // A sub-texture
  struct Texture
  {
    /* Clamp mode
       0 - clampS clampT
       1 - clampS wrapT
       2 - wrapS clampT
       3 - wrapS wrapT
    */

    /* Filter:
       0 - nearest
       1 - bilinear
       2 - trilinear
       3, 4, 5 - who knows
    */
    bool inUse;
    NiSourceTexturePtr texture;

    int clamp, set, filter;
    short unknown2;

    void read(NIFFile *nif)
    {
      inUse = nif->getInt();
      if(!inUse) return;

      texture.read(nif);
      clamp = nif->getInt();
      filter = nif->getInt();
      set = nif->getInt();

      // I have no idea, but I think these are actually two
      // PS2-specific shorts (ps2L and ps2K), followed by an unknown
      // short.
      nif->skip(6);
    }
  };

  /* Apply mode:
     0 - replace
     1 - decal
     2 - modulate
     3 - hilight  // These two are for PS2 only?
     4 - hilight2
  */
  int apply;

  /*
   * The textures in this list are as follows:
   *
   * 0 - Base texture
   * 1 - Dark texture
   * 2 - Detail texture
   * 3 - Gloss texture (never used?)
   * 4 - Glow texture
   * 5 - Bump map texture
   * 6 - Decal texture
   */
  Texture textures[7];

  void read(NIFFile *nif)
  {
    Property::read(nif);
    apply = nif->getInt();

    // Unknown, always 7. Probably the number of textures to read
    // below
    nif->getInt();

    textures[0].read(nif); // Base
    textures[1].read(nif); // Dark
    textures[2].read(nif); // Detail
    textures[3].read(nif); // Gloss (never present)
    textures[4].read(nif); // Glow
    textures[5].read(nif); // Bump map
    if(textures[5].inUse)
      {
        // Ignore these at the moment
        float lumaScale = nif->getFloat();
        float lumaOffset = nif->getFloat();
        const Vector4 *lumaMatrix = nif->getVector4();
      }
    textures[6].read(nif); // Decal
  }
};

} // Namespace
#endif
