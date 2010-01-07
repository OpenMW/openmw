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

struct NiAutoNormalParticlesData : ShapeData
{
  int activeCount;

  void read(NIFFile *nif)
  {
    ShapeData::read(nif);

    // Should always match the number of vertices
    activeCount = nif->getUshort();

    // Skip all the info, we don't support particles yet
    nif->getFloat();  // Active radius ?
    nif->getUshort(); // Number of valid entries in the following arrays ?

    if(nif->getInt())
      // Particle sizes
      nif->getFloatLen(activeCount);
  }
};

struct NiRotatingParticlesData : NiAutoNormalParticlesData
{
  void read(NIFFile *nif)
  {
    NiAutoNormalParticlesData::read(nif);

    if(nif->getInt())
      // Rotation quaternions. I THINK activeCount is correct here,
      // but verts (vertex number) might also be correct, if there is
      // any case where the two don't match.
      nif->getArrayLen<Vector4>(activeCount);
  }
};

struct NiPosData : Record
{
  void read(NIFFile *nif)
  {
    int count = nif->getInt();
    int type = nif->getInt();
    if(type != 1 && type != 2)
      fail("Cannot handle NiPosData type");

    // TODO: Could make structs of these. Seems to be identical to
    // translation in NiKeyframeData.
    for(int i=0; i<count; i++)
      {
        float time = nif->getFloat();
        nif->getVector(); // This isn't really shared between type 1
                          // and type 2, most likely
        if(type == 2)
          {
            nif->getVector();
            nif->getVector();
          }
      }
  }
}

struct NiUVData : Record
{
  void read(NIFFile *nif)
  {
    // TODO: This is claimed to be a "float animation key", which is
    // also used in FloatData and KeyframeData. We could probably
    // reuse and refactor a lot of this if we actually use it at some
    // point.

    for(int i=0; i<2; i++)
      {
        int count = nif->getInt();

        if(count)
          {
            nif->getInt(); // always 2
            nif->getArrayLen<Vector4>(count); // Really one time float + one vector
          }
      }
    // Always 0
    nif->getInt();
    nif->getInt();
  }
};

struct NiFloatData : Record
{
  void read(NIFFile *nif)
  {
    int count = nif->getInt();
    nif->getInt(); // always 2
    nif->getArrayLen<Vector4>(count); // Really one time float + one vector
  }
};

struct NiPixelData : Record
{
  unsigned int rmask, gmask, bmask, amask;
  int bpp, mips;

  void read(NIFFile *nif)
  {
    nif->getInt(); // always 0 or 1

    rmask = nif->getInt(); // usually 0xff
    gmask = nif->getInt(); // usually 0xff00
    bmask = nif->getInt(); // usually 0xff0000
    amask = nif->getInt(); // usually 0xff000000 or zero

    bpp = nif->getInt();

    // Unknown
    nif->skip(12);

    mips = nif->getInt();

    // Bytes per pixel, should be bpp * 8
    int bytes = nif->getInt();

    for(int i=0; i<mips; i++)
      {
        // Image size and offset in the following data field
        int x = nif->getInt();
        int y = nif->getInt();
        int offset = nif->getInt();
      }

    // Skip the data
    unsigned int dataSize = nif->getInt();
    nif->skip(dataSize);
  }
};

struct NiColorData : Record
{
  struct ColorData
  {
    float time;
    Vector4 rgba;
  };

  void read(NIFFile *nif)
  {
    int count = nif->getInt();
    nif->getInt(); // always 1

    // Skip the data
    assert(ColorData.sizeof = 4*5);
    nif->skip(ColorData.sizeof * count);
  }
};

struct NiVisData : Record
{
  void read(NIFFile *nif)
  {
    int count = nif->getInt();
    /*
      Each VisData consists of:
        float time;
        byte isSet;

      If you implement this, make sure you use a packed struct
      (sizeof==5), or read each element individually.
     */
    nif->skip(count*5);
  }
};

struct NiSkinData : Record
{
  void read(NIFFile *nif)
  {
  }
};

struct NiMorphData : Record
{
  void read(NIFFile *nif)
  {
  }
};

struct NiKeyframeData : Record
{
  void read(NIFFile *nif)
  {
  }
};

} // Namespace
#endif
