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
struct NiGeometryData : public Record
{
    std::vector<osg::Vec3f> vertices, normals, tangents, bitangents;
    std::vector<osg::Vec4f> colors;
    std::vector< std::vector<osg::Vec2f> > uvlist;
    osg::Vec3f center;
    float radius;

    void read(NIFStream *nif) override;
};

struct NiTriShapeData : public NiGeometryData
{
    // Triangles, three vertex indices per triangle
    std::vector<unsigned short> triangles;

    void read(NIFStream *nif) override;
};

struct NiTriStripsData : public NiGeometryData
{
    // Triangle strips, series of vertex indices.
    std::vector<std::vector<unsigned short>> strips;

    void read(NIFStream *nif) override;
};

struct NiLinesData : public NiGeometryData
{
    // Lines, series of indices that correspond to connected vertices.
    std::vector<unsigned short> lines;

    void read(NIFStream *nif) override;
};

struct NiParticlesData : public NiGeometryData
{
    int numParticles{0};

    int activeCount{0};

    std::vector<float> particleRadii, sizes, rotationAngles;
    std::vector<osg::Quat> rotations;
    std::vector<osg::Vec3f> rotationAxes;

    void read(NIFStream *nif) override;
};

struct NiRotatingParticlesData : public NiParticlesData
{
    void read(NIFStream *nif) override;
};

struct NiPosData : public Record
{
    Vector3KeyMapPtr mKeyList;

    void read(NIFStream *nif) override;
};

struct NiUVData : public Record
{
    FloatKeyMapPtr mKeyList[4];

    void read(NIFStream *nif) override;
};

struct NiFloatData : public Record
{
    FloatKeyMapPtr mKeyList;

    void read(NIFStream *nif) override;
};

struct NiPixelData : public Record
{
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
    Format fmt{NIPXFMT_RGB8};

    unsigned int colorMask[4]{0};
    unsigned int bpp{0}, pixelTiling{0};
    bool sRGB{false};

    NiPalettePtr palette;
    unsigned int numberOfMipmaps{0};

    struct Mipmap
    {
        int width, height;
        int dataOffset;
    };
    std::vector<Mipmap> mipmaps;

    std::vector<unsigned char> data;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiColorData : public Record
{
    Vector4KeyMapPtr mKeyMap;

    void read(NIFStream *nif) override;
};

struct NiVisData : public Record
{
    struct VisData {
        float time;
        bool isSet;
    };
    std::vector<VisData> mVis;

    void read(NIFStream *nif) override;
};

struct NiSkinInstance : public Record
{
    NiSkinDataPtr data;
    NiSkinPartitionPtr partitions;
    NodePtr root;
    NodeList bones;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiSkinData : public Record
{
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
    NiSkinPartitionPtr partitions;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

struct NiSkinPartition : public Record
{
    struct Partition
    {
        std::vector<unsigned short> bones;
        std::vector<unsigned short> vertexMap;
        std::vector<float> weights;
        std::vector<std::vector<unsigned short>> strips;
        std::vector<unsigned short> triangles;
        std::vector<char> boneIndices;
        void read(NIFStream *nif);
    };
    std::vector<Partition> data;

    void read(NIFStream *nif) override;
};

struct NiMorphData : public Record
{
    struct MorphData {
        FloatKeyMapPtr mKeyFrames;
        std::vector<osg::Vec3f> mVertices;
    };
    std::vector<MorphData> mMorphs;

    void read(NIFStream *nif) override;
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

    void read(NIFStream *nif) override;
};

struct NiPalette : public Record
{
    // 32-bit RGBA colors that correspond to 8-bit indices
    std::vector<unsigned int> colors;

    void read(NIFStream *nif) override;
};

struct NiStringPalette : public Record
{
    std::string palette;
    void read(NIFStream *nif) override;
};

struct NiBoolData : public Record
{
    ByteKeyMapPtr mKeyList;
    void read(NIFStream *nif) override;
};

} // Namespace
#endif
