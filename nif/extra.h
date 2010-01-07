/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (extra.h) is part of the OpenMW package.

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

#ifndef _NIF_EXTRA_H_
#define _NIF_EXTRA_H_

#include "record.h"
#include "nif_file.h"
#include "record_ptr.h"

namespace Nif
{

/** A record that can have extra data. The extra data objects
    themselves decend from the Extra class, and all the extra data
    connected to an object form a linked list
*/
struct Extra : Record
{
  ExtraPtr extra;

  void read(NIFFile *nif) { extra.read(nif); }
};

struct NiVertWeigthsExtraData : Extra
{
  void read(NIFFile *nif)
  {
    Extra::read(nif);

    // We should have s*4+2 == i, for some reason. Might simply be the
    // size of the rest of the record, unhelpful as that may be.
    int i = nif->getInt();
    int s = nif->getShort();

    nif->getFloatLen(s);
  }
};

struct NiTextKeyExtraData : Extra
{
  void read(NIFFile *nif)
  {
    Extra::read(nif);

    nif->getInt(); // 0

    int keynum = nif->getInt();
    for(int i=0; i<keynum; i++)
      {
        nif->getFloat();  // time
        nif->getString(); // key text
      }
  }
};

struct NiStringExtraData : Extra
{
  /* Two known meanings:
     "MRK" - marker, only visible in the editor, not rendered in-game
     "NCO" - no collision
   */
  SString string;

  void read(NIFFile *nif)
  {
    Extra::read(nif);

    nif->getInt(); // size of string + 4. Really useful...
    string = nif->getString();
  }
};

struct NiParticleGrowFade : Extra
{
  void read(NIFFile *nif)
  {
    Extra::read(nif);

    // Two floats.
    nif->skip(8);
  }
};

struct NiParticleColorModifier : Extra
{
  NiColorDataPtr data;

  void read(NIFFile *nif)
  {
    Extra::read(nif);
    data.read(nif);
  }
};

struct NiGravity : Extra
{
  void read(NIFFile *nif)
  {
    Extra::read(nif);

    // two floats, one int, six floats
    nif->skip(9*4);
  }
};

// NiPinaColada
struct NiPlanarCollider : Extra
{
  void read(NIFFile *nif)
  {
    Extra::read(nif);

    // (I think) 4 floats + 4 vectors
    nif->skip(4*16);
  }
};

struct NiParticleRotation : Extra
{
  void read(NIFFile *nif)
  {
    Extra::read(nif);

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
