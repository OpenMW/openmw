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

#include "controlled.hpp"
#include <iostream>

namespace Nif
{

class NiSourceTexture : public Named
{
public:

  // Is this an external (references a separate texture file) or
  // internal (data is inside the nif itself) texture?
  bool external;

  Misc::SString filename;    // In case of external textures
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

    external = !!nif->getByte();

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
class ShapeData : public Record
{
public:
  Misc::FloatArray vertices, normals, colors, uvlist;
  const Vector *center;
  float radius;

  void read(NIFFile *nif)
  {
    int verts = nif->getShort();

    if(nif->getInt())
      vertices = nif->getFloatLen(verts*3);

    if(nif->getInt())
      normals = nif->getFloatLen(verts*3);

    center = nif->getVector();
    radius = nif->getFloat();

    if(nif->getInt())
      colors = nif->getFloatLen(verts*4);

    int uvs = nif->getShort();

    // Only the first 6 bits are used as a count. I think the rest are
    // flags of some sort.
    uvs &= 0x3f;

    if(nif->getInt())
      uvlist = nif->getFloatLen(uvs*verts*2);
  }
};

class NiTriShapeData : public ShapeData
{
public:
  // Triangles, three vertex indices per triangle
  Misc::SliceArray<short> triangles;

  void read(NIFFile *nif)
  {
    ShapeData::read(nif);

    int tris = nif->getShort();
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
    int verts = nif->getShort();
    if(verts)
      {
        for(int i=0;i<verts;i++)
          {
            // Number of vertices matching vertex 'i'
            short num = nif->getShort();
            nif->skip(num*sizeof(short));
          }
      }
  }
};

class NiAutoNormalParticlesData : public ShapeData
{
public:
  int activeCount;

  void read(NIFFile *nif)
  {
    ShapeData::read(nif);

    // Should always match the number of vertices
    activeCount = nif->getShort();

    // Skip all the info, we don't support particles yet
    nif->getFloat();  // Active radius ?
    nif->getShort(); // Number of valid entries in the following arrays ?

    if(nif->getInt())
      // Particle sizes
      nif->getFloatLen(activeCount);
  }
};

class NiRotatingParticlesData : public NiAutoNormalParticlesData
{
public:
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

class NiPosData : public Record
{
public:
  void read(NIFFile *nif)
  {
    int count = nif->getInt();
    int type = nif->getInt();
    if(type != 1 && type != 2)
      nif->fail("Cannot handle NiPosData type");

    // TODO: Could make structs of these. Seems to be identical to
    // translation in NiKeyframeData.
    for(int i=0; i<count; i++)
      {
        /*float time =*/ nif->getFloat();
        nif->getVector(); // This isn't really shared between type 1
                          // and type 2, most likely
        if(type == 2)
          {
            nif->getVector();
            nif->getVector();
          }
      }
  }
};

class NiUVData : public Record
{
public:
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

class NiFloatData : public Record
{
public:
  void read(NIFFile *nif)
  {
    int count = nif->getInt();
    nif->getInt(); // always 2
    nif->getArrayLen<Vector4>(count); // Really one time float + one vector
  }
};

class NiPixelData : public Record
{
public:
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
    /*int bytes =*/ nif->getInt();

    for(int i=0; i<mips; i++)
      {
        // Image size and offset in the following data field
        /*int x =*/ nif->getInt();
        /*int y =*/ nif->getInt();
        /*int offset =*/ nif->getInt();
      }

    // Skip the data
    unsigned int dataSize = nif->getInt();
    nif->skip(dataSize);
  }
};

class NiColorData : public Record
{
public:
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
    assert(sizeof(ColorData) == 4*5);
    nif->skip(sizeof(ColorData) * count);
  }
};

class NiVisData : public Record
{
public:
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

class NiSkinInstance : public Record
{
public:
  NiSkinDataPtr data;
  NodePtr root;
  NodeList bones;

  void read(NIFFile *nif)
  {
    data.read(nif);
    root.read(nif);
    bones.read(nif);

    if(data.empty() || root.empty())
      nif->fail("NiSkinInstance missing root or data");
  }

  void post(NIFFile *nif);
};

class NiSkinData : public Record
{
public:
  // This is to make sure the structs are packed, ie. that the
  // compiler doesn't mess them up with extra alignment bytes.
#pragma pack(push)
#pragma pack(1)

  struct BoneTrafo
  {
    Matrix rotation; // Rotation offset from bone?
    Vector trans;    // Translation
    float scale;     // Probably scale (always 1)
  };

  struct VertWeight
  {
    short vertex;
    float weight;
  };
#pragma pack(pop)

  struct BoneInfo
  {
    const BoneTrafo *trafo;
    const Vector4 *unknown;
    Misc::SliceArray<VertWeight> weights;
  };

  const BoneTrafo *trafo;
  std::vector<BoneInfo> bones;

  void read(NIFFile *nif)
  {
    assert(sizeof(BoneTrafo) == 4*(9+3+1));
    assert(sizeof(VertWeight) == 6);

    trafo = nif->getPtr<BoneTrafo>();

    int boneNum = nif->getInt();
    nif->getInt(); // -1

    bones.resize(boneNum);

    for(int i=0;i<boneNum;i++)
      {
        BoneInfo &bi = bones[i];

        bi.trafo = nif->getPtr<BoneTrafo>();
        bi.unknown = nif->getVector4();

        // Number of vertex weights
        int count = nif->getShort();
        bi.weights = nif->getArrayLen<VertWeight>(count);
      }
  }
};

class NiMorphData : public Record
{
public:
  void read(NIFFile *nif)
  {
    int morphCount = nif->getInt();
    int vertCount  = nif->getInt();
    nif->getByte();

    for(int i=0; i<morphCount; i++)
      {
        int magic = nif->getInt();
        nif->getInt();
        if(magic)
          // Time, data, forward, backward tangents
          nif->getFloatLen(4*magic);

        nif->getFloatLen(vertCount*3);
      }
  }
};

class NiKeyframeData : public Record
{
    public:

        void read(NIFFile *nif)
        {
            // Rotations first
            int count = nif->getInt();
            if(count)
            {
                int type = nif->getInt();

                if(type == 1)
                    nif->skip(count*4*5); // time + quaternion
                else if(type == 3)
                    nif->skip(count*4*8); // rot1 + tension+bias+continuity
                else if(type == 4)
                {
                    for(int j=0;j<count;j++)
                    {
                        nif->getFloat(); // time
                        for(int i=0; i<3; i++)
                        {
                            int cnt = nif->getInt();
                            int type = nif->getInt();
                            if(type == 1)
                                nif->skip(cnt*4*2); // time + unknown
                            else if(type == 2)
                                nif->skip(cnt*4*4); // time + unknown vector
                            else nif->fail("Unknown sub-rotation type");
                        }
                    }
                }
                else nif->fail("Unknown rotation type in NiKeyframeData");
            }

            // Then translation
            count = nif->getInt();

            if(count)
            {
                int type = nif->getInt();

                if(type == 1)
                    nif->getFloatLen(count*4); // time + translation
                else if(type == 2)
                    nif->getFloatLen(count*10); // trans1 + forward + backward
                else if(type == 3)
                    nif->getFloatLen(count*7); // trans1 + tension,bias,continuity
                else nif->fail("Unknown translation type");
            }

            // Finally, scalings
            count = nif->getInt();
            if(count)
            {
                int type = nif->getInt();

                int size = 0;
                if(type == 1) size = 2; // time+scale
                else if(type == 2) size = 4; // 1 + forward + backward (floats)
                else if(type == 3) size = 5; // 1 + tbc
                else nif->fail("Unknown scaling type");
                nif->getFloatLen(count*size);
            }
        }
};

} // Namespace
#endif
