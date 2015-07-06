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

#include "niftypes.hpp" // Transformation

#include <osg/Array>

namespace Nif
{

// Common ancestor for several data classes
class ShapeData : public Record
{
public:
    osg::ref_ptr<osg::Vec3Array> vertices, normals;
    osg::ref_ptr<osg::Vec4Array> colors;

    std::vector< osg::ref_ptr<osg::Vec2Array> > uvlist;
    osg::Vec3f center;
    float radius;

    void read(NIFStream *nif);
};

class NiTriShapeData : public ShapeData
{
public:
    // Triangles, three vertex indices per triangle
    osg::ref_ptr<osg::DrawElementsUShort> triangles;

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
    unsigned int rmask, gmask, bmask, amask;
    int bpp, mips;

    void read(NIFStream *nif);
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
        osg::ref_ptr<osg::Vec3Array> mVertices;
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

} // Namespace
#endif
