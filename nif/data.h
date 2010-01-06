/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (data.h) is part of the OpenMW package.

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

#ifndef _NIF_DATA_H_
#define _NIF_DATA_H_

#include "controlled.h"

namespace Nif
{

struct NiSourceTexture : Named
{
  // Is this an external (references a separate texture file) or
  // internal (data is inside the nif itself) texture?
  bool external;

  SString filename;    // In case of external textures
  NiPixelDataPtr data; // In case of internal textures

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

  void read(NIFFile *nif)
  {
    Named::read(nif);

    external = nif->getByte();

    if(external) filename = nif->getString();
    else
      {
        nif->getByte(); // always 1
        data.read(nif);
      }

    pixel = nif->getInt();
    mipmap = nif->getInt();
    alpha = nif->getInt();

    nif->getByte(); // always 1
  }
};

// Common ancestor for several data classes
struct ShapeData : Record
{
  FloatArray vertices, normals, colors, uvlist;
  const Vector *center;
  float radius;

  void read(NIFFile *nif)
  {
    int verts = nif->getUshort();

    if(nif->getInt())
      vertices = nif->getFloatLen(verts*3);

    if(nif->getInt())
      normals = nif->getFloatLen(verts*3);

    center = nif->getVector();
    radius = nif->getFloat();

    if(nif->getInt())
      colors = nif->getFloatLen(verts*4);

    int uvs = nif->getUshort();

    // Only the first 6 bits are used as a count. I think the rest are
    // flags of some sort.
    uvs &= 0x3f;

    if(nif->getInt())
      uvlist = nif->getFloatLen(uvs*verts*2);
  }
};

struct NiTriShapeData : ShapeData
{
  // Triangles, three vertex indices per triangle
  SliceArray<short> triangles;

  void read(NIFFile *nif)
  {
    ShapeData::read(nif);

    int tris = nif->getUshort();
    if(tris)
      {
        // We have three times as many vertices as triangles, so this
        // is always equal to tris*3.
        int cnt = nif->getInt();
        triangles = nif->getArrayLen<short>(cnt);
      }

    // Read the match list, which lists the vertices that are equal to
    // vertices. We don't actually need need this for anything, so
    // just skip it.
    int verts = nif->getUshort();
    if(verts)
      {
        for(int i=0;i<verts;i++)
          {
            // Number of vertices matching vertex 'i'
            short num = nif->getUshort();
            nif->skip(num*sizeof(short));
          }
      }
  }
};

} // Namespace
#endif
