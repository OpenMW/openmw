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

#ifndef OPENMW_COMPONENTS_NIF_DATA_HPP
#define OPENMW_COMPONENTS_NIF_DATA_HPP

#include "base.hpp"

namespace Nif
{

// Common ancestor for several data classes
class ShapeData : public Record
{
public:
    std::vector<Ogre::Vector3> vertices, normals;
    std::vector<Ogre::Vector4> colors;
    std::vector< std::vector<Ogre::Vector2> > uvlist;
    Ogre::Vector3 center;
    float radius;

    void read(NIFStream *nif)
    {
        int verts = nif->getUShort();

        if(nif->getInt())
            nif->getVector3s(vertices, verts);

        if(nif->getInt())
            nif->getVector3s(normals, verts);

        center = nif->getVector3();
        radius = nif->getFloat();

        if(nif->getInt())
            nif->getVector4s(colors, verts);

        // Only the first 6 bits are used as a count. I think the rest are
        // flags of some sort.
        int uvs = nif->getUShort();
        uvs &= 0x3f;

        if(nif->getInt())
        {
            uvlist.resize(uvs);
            for(int i = 0;i < uvs;i++)
                nif->getVector2s(uvlist[i], verts);
        }
    }
};

class NiTriShapeData : public ShapeData
{
public:
    // Triangles, three vertex indices per triangle
    std::vector<short> triangles;

    void read(NIFStream *nif)
    {
        ShapeData::read(nif);

        /*int tris =*/ nif->getUShort();

        // We have three times as many vertices as triangles, so this
        // is always equal to tris*3.
        int cnt = nif->getInt();
        nif->getShorts(triangles, cnt);

        // Read the match list, which lists the vertices that are equal to
        // vertices. We don't actually need need this for anything, so
        // just skip it.
        int verts = nif->getUShort();
        for(int i=0;i < verts;i++)
        {
            // Number of vertices matching vertex 'i'
            int num = nif->getUShort();
            nif->skip(num * sizeof(short));
        }
    }
};

class NiAutoNormalParticlesData : public ShapeData
{
public:
    int numParticles;

    float particleRadius;

    int activeCount;

    std::vector<float> sizes;

    void read(NIFStream *nif)
    {
        ShapeData::read(nif);

        // Should always match the number of vertices
        numParticles = nif->getUShort();

        particleRadius = nif->getFloat();
        activeCount = nif->getUShort();

        if(nif->getInt())
        {
            // Particle sizes
            nif->getFloats(sizes, vertices.size());
        }
    }
};

class NiRotatingParticlesData : public NiAutoNormalParticlesData
{
public:
    std::vector<Ogre::Quaternion> rotations;

    void read(NIFStream *nif)
    {
        NiAutoNormalParticlesData::read(nif);

        if(nif->getInt())
        {
            // Rotation quaternions.
            nif->getQuaternions(rotations, vertices.size());
        }
    }
};

class NiPosData : public Record
{
public:
    Vector3KeyMap mKeyList;

    void read(NIFStream *nif)
    {
        mKeyList.read(nif);
    }
};

class NiUVData : public Record
{
public:
    FloatKeyMap mKeyList[4];

    void read(NIFStream *nif)
    {
        for(int i = 0;i < 4;i++)
            mKeyList[i].read(nif);
    }
};

class NiFloatData : public Record
{
public:
    FloatKeyMap mKeyList;

    void read(NIFStream *nif)
    {
        mKeyList.read(nif);
    }
};

class NiPixelData : public Record
{
public:
    unsigned int rmask, gmask, bmask, amask;
    int bpp, mips;

    void read(NIFStream *nif)
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
    Vector4KeyMap mKeyMap;

    void read(NIFStream *nif)
    {
        mKeyMap.read(nif);
    }
};

class NiVisData : public Record
{
public:
    struct VisData {
        float time;
        bool isSet;
    };
    std::vector<VisData> mVis;

    void read(NIFStream *nif)
    {
        int count = nif->getInt();
        mVis.resize(count);
        for(size_t i = 0;i < mVis.size();i++)
        {
            mVis[i].time = nif->getFloat();
            mVis[i].isSet = nif->getChar() != 0;
        }
    }
};

class NiSkinInstance : public Record
{
public:
    NiSkinDataPtr data;
    NodePtr root;
    NodeList bones;

    void read(NIFStream *nif)
    {
        data.read(nif);
        root.read(nif);
        bones.read(nif);
    }

    void post(NIFFile *nif);
};

class NiSkinData : public Record
{
public:
    struct BoneTrafo
    {
        Ogre::Matrix3 rotation; // Rotation offset from bone?
        Ogre::Vector3 trans;    // Translation
        float scale;            // Probably scale (always 1)
    };

    struct VertWeight
    {
        short vertex;
        float weight;
    };

    struct BoneInfo
    {
        BoneTrafo trafo;
        Ogre::Vector4 unknown;
        std::vector<VertWeight> weights;
    };

    BoneTrafo trafo;
    std::vector<BoneInfo> bones;

    void read(NIFStream *nif)
    {
        trafo.rotation = nif->getMatrix3();
        trafo.trans = nif->getVector3();
        trafo.scale = nif->getFloat();

        int boneNum = nif->getInt();
        nif->getInt(); // -1

        bones.resize(boneNum);
        for(int i=0;i<boneNum;i++)
        {
            BoneInfo &bi = bones[i];

            bi.trafo.rotation = nif->getMatrix3();
            bi.trafo.trans = nif->getVector3();
            bi.trafo.scale = nif->getFloat();
            bi.unknown = nif->getVector4();

            // Number of vertex weights
            bi.weights.resize(nif->getUShort());
            for(size_t j = 0;j < bi.weights.size();j++)
            {
                bi.weights[j].vertex = nif->getUShort();
                bi.weights[j].weight = nif->getFloat();
            }
        }
    }
};

struct NiMorphData : public Record
{
    struct MorphData {
        FloatKeyMap mData;
        std::vector<Ogre::Vector3> mVertices;
    };
    std::vector<MorphData> mMorphs;

    void read(NIFStream *nif)
    {
        int morphCount = nif->getInt();
        int vertCount  = nif->getInt();
        /*relative targets?*/nif->getChar();

        mMorphs.resize(morphCount);
        for(int i = 0;i < morphCount;i++)
        {
            mMorphs[i].mData.read(nif, true);
            nif->getVector3s(mMorphs[i].mVertices, vertCount);
        }
    }
};


struct NiKeyframeData : public Record
{
    QuaternionKeyMap mRotations;

    FloatKeyMap mXRotations;
    FloatKeyMap mYRotations;
    FloatKeyMap mZRotations;

    Vector3KeyMap mTranslations;
    FloatKeyMap mScales;

    void read(NIFStream *nif)
    {
        mRotations.read(nif);
        if(mRotations.mInterpolationType == mRotations.sXYZInterpolation)
        {
            //Chomp unused float
            nif->getFloat();
            mXRotations.read(nif, true);
            mYRotations.read(nif, true);
            mZRotations.read(nif, true);
        }
        mTranslations.read(nif);
        mScales.read(nif);
    }
};

} // Namespace
#endif
