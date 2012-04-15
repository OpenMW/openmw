/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (nif_types.h) is part of the OpenMW package.

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

#ifndef _NIF_TYPES_H_
#define _NIF_TYPES_H_

// Common types used in NIF files

namespace Nif
{

/* These packing #pragmas aren't really necessary on 32 bit
   machines. I haven't tested on 64 bit yet. In any case it doesn't
   hurt to include them. We can't allow any compiler-generated padding
   in any of these structs, since they are used to interface directly
   with raw data from the NIF files.
*/
#pragma pack(push)
#pragma pack(1)

struct Vector
{
  float array[3];
};

struct Vector4
{
  float array[4];
};

struct Matrix
{
  Vector v[3];
};

struct Transformation
{
    Vector pos;
    Matrix rotation;
    float scale;
    Vector velocity;

    static const Transformation* getIdentity()
    {
        static Transformation identity;
        static bool iset = false;
        if (!iset)
        {
            identity.scale = 1.0f;
            identity.rotation.v[0].array[0] = 1.0f;
            identity.rotation.v[1].array[1] = 1.0f;
            identity.rotation.v[2].array[2] = 1.0f;
            iset = true;
        }

        return &identity;
    }
};
#pragma pack(pop)

} // Namespace
#endif
