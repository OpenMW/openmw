/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: https://openmw.org/

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
  https://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_DATA_HPP
#define OPENMW_COMPONENTS_NIF_DATA_HPP

#include "base.hpp"

#include "niftypes.hpp" // Transformation

namespace Nif
{

// Common ancestor for several data classes
class ShapeData : public Record
{
public:
    std::vector<osg::Vec3f> vertices, normals;
    std::vector<osg::Vec4f> colors;
    std::vector< std::vector<osg::Vec2f> > uvlist;
    osg::Vec3f center;
    float radius;

    void read(NIFStream *nif);
};

class NiTriShapeData : public ShapeData
{
public:
    // Triangles, three vertex indices per triangle
    std::vector<unsigned short> triangles;

    void read(NIFStream *nif);
};

class NiTriStripsData : public ShapeData
{
public:
    // Triangle strips, series of vertex indices.
    std::vector<std::vector<unsigned short>> strips;

    void read(NIFStream *nif);
};

class NiAutoNormalParticlesData : public ShapeData
{
public:
    int numParticles;

    float particleRadius;

    int activeCount;

    std::vector<float> sizes;

    void read(NIFStream *nif);
};

class NiRotatingParticlesData : public NiAutoNormalParticlesData
{
public:
    std::vector<osg::Quat> rotations;

    void read(NIFStream *nif);
};

class NiPosData : public Record
{
public:
    Vector3KeyMapPtr mKeyList;

    void read(NIFStream *nif);
};

class NiUVData : public Record
{
public:
    FloatKeyMapPtr mKeyList[4];

    void read(NIFStream *nif);
};

class NiFloatData : public Record
{
public:
    FloatKeyMapPtr mKeyList;

    void read(NIFStream *nif);
};

class NiPixelData : public Record
{
public:
    enum Format
    {
        NIPXFMT_RGB8,
        NIPXFMT_RGBA8,
        NIPXFMT_PAL8,
        NIPXFMT_PALA8,
        NIPXFMT_DXT1,
        NIPXFMT_DXT3,
        NIPXFMT_DXT5,
        NIPXFMT_DXT5_ALT
    };
    Format fmt;

    unsigned int rmask, gmask, bmask, amask, bpp;

    NiPalettePtr palette;
    unsigned int numberOfMipmaps;

    struct Mipmap
    {
        int width, height;
        int dataOffset;
    };
    std::vector<Mipmap> mipmaps;

    std::vector<unsigned char> data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiColorData : public Record
{
public:
    Vector4KeyMapPtr mKeyMap;

    void read(NIFStream *nif);
};

class NiVisData : public Record
{
public:
    struct VisData {
        float time;
        bool isSet;
    };
    std::vector<VisData> mVis;

    void read(NIFStream *nif);
};

class NiSkinInstance : public Record
{
public:
    NiSkinDataPtr data;
    NodePtr root;
    NodeList bones;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiSkinData : public Record
{
public:
    struct VertWeight
    {
        unsigned short vertex;
        float weight;
    };

    struct BoneInfo
    {
        Transformation trafo;
        osg::Vec3f boundSphereCenter;
        float boundSphereRadius;
        std::vector<VertWeight> weights;
    };

    Transformation trafo;
    std::vector<BoneInfo> bones;

    void read(NIFStream *nif);
};

struct NiMorphData : public Record
{
    struct MorphData {
        FloatKeyMapPtr mKeyFrames;
        std::vector<osg::Vec3f> mVertices;
    };
    std::vector<MorphData> mMorphs;

    void read(NIFStream *nif);
};


struct NiKeyframeData : public Record
{
    QuaternionKeyMapPtr mRotations;

    // may be NULL
    FloatKeyMapPtr mXRotations;
    FloatKeyMapPtr mYRotations;
    FloatKeyMapPtr mZRotations;

    Vector3KeyMapPtr mTranslations;
    FloatKeyMapPtr mScales;

    void read(NIFStream *nif);
};

class NiPalette : public Record
{
public:
    // 32-bit RGBA colors that correspond to 8-bit indices
    std::vector<unsigned int> colors;

    void read(NIFStream *nif);
};

} // Namespace
#endif
