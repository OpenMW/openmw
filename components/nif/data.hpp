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

#include <OgreQuaternion.h>
#include <OgreVector3.h>

namespace Nif
{

class NiSourceTexture : public Named
{
public:
    // Is this an external (references a separate texture file) or
    // internal (data is inside the nif itself) texture?
    bool external;

    std::string filename; // In case of external textures
    NiPixelDataPtr data;  // In case of internal textures

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
        if(external)
            filename = nif->getString();
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

    void post(NIFFile *nif)
    {
        Named::post(nif);
        data.post(nif);
    }
};

// Common ancestor for several data classes
class ShapeData : public Record
{
public:
    std::vector<float> vertices, normals, colors, uvlist;
    Ogre::Vector3 center;
    float radius;

    void read(NIFFile *nif)
    {
        int verts = nif->getShort();

        if(nif->getInt())
            nif->load(vertices, verts*3);

        if(nif->getInt())
            nif->load(normals, verts*3);

        center = nif->getVector();
        radius = nif->getFloat();

        if(nif->getInt())
            nif->load(colors, verts*4);

        // Only the first 6 bits are used as a count. I think the rest are
        // flags of some sort.
        int uvs = nif->getShort();
        uvs &= 0x3f;

        if(nif->getInt())
            nif->load(uvlist, uvs*verts*2);
    }
};

class NiTriShapeData : public ShapeData
{
public:
    // Triangles, three vertex indices per triangle
    std::vector<short> triangles;

    void read(NIFFile *nif)
    {
        ShapeData::read(nif);

        int tris = nif->getShort();
        if(tris)
        {
            // We have three times as many vertices as triangles, so this
            // is always equal to tris*3.
            int cnt = nif->getInt();
            nif->load(triangles, cnt);
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
        nif->getFloat(); // Active radius ?
        nif->getShort(); // Number of valid entries in the following arrays ?

        if(nif->getInt())
        {
            // Particle sizes
            nif->skip(activeCount * sizeof(float));
        }
    }
};

class NiRotatingParticlesData : public NiAutoNormalParticlesData
{
public:
    void read(NIFFile *nif)
    {
        NiAutoNormalParticlesData::read(nif);

        if(nif->getInt())
        {
            // Rotation quaternions. I THINK activeCount is correct here,
            // but verts (vertex number) might also be correct, if there is
            // any case where the two don't match.
            nif->skip(activeCount * 4*sizeof(float));
        }
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
                nif->skip(count * (sizeof(float) + 3*sizeof(float))); // Really one time float + one vector
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
        nif->skip(count * (sizeof(float) + 3*sizeof(float))); // Really one time float + one vector
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
        Ogre::Vector4 rgba;
    };

    void read(NIFFile *nif)
    {
        int count = nif->getInt();
        nif->getInt(); // always 1

        // Skip the data
        nif->skip(count * 5*sizeof(float));
    }
};

class NiVisData : public Record
{
public:
    struct VisData {
        float time;
        char isSet;
    };

    void read(NIFFile *nif)
    {
        int count = nif->getInt();

        /* Skip VisData */
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
    struct BoneTrafoCopy
    {
        Ogre::Quaternion rotation;
        Ogre::Vector3 trans;
        float scale;
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
    struct BoneInfoCopy
    {
        std::string bonename;
        unsigned short bonehandle;
        BoneTrafoCopy trafo;
        Ogre::Vector4 unknown;
        //std::vector<VertWeight> weights;
    };
    struct IndividualWeight
    {
        float weight;
        unsigned int boneinfocopyindex;
    };

    BoneTrafo trafo;
    std::vector<BoneInfo> bones;

    void read(NIFFile *nif)
    {
        trafo.rotation = nif->getMatrix();
        trafo.trans = nif->getVector();
        trafo.scale = nif->getFloat();

        int boneNum = nif->getInt();
        nif->getInt(); // -1

        bones.resize(boneNum);
        for(int i=0;i<boneNum;i++)
        {
            BoneInfo &bi = bones[i];

            bi.trafo.rotation = nif->getMatrix();
            bi.trafo.trans = nif->getVector();
            bi.trafo.scale = nif->getFloat();
            bi.unknown = nif->getVector4();

            // Number of vertex weights
            bi.weights.resize(nif->getShort());
            for(size_t j = 0;j < bi.weights.size();j++)
            {
                nif->load(bi.weights[j].vertex);
                nif->load(bi.weights[j].weight);
            }
        }
    }
};

class NiMorphData : public Record
{
    float startTime;
    float stopTime;
    std::vector<Ogre::Vector3> initialVertices;
    std::vector<std::vector<float> > relevantTimes;
    std::vector<std::vector<Ogre::Vector3> > relevantData;
    std::vector<std::vector<Ogre::Vector3> > additionalVertices;

public:
    float getStartTime() const
    { return startTime; }
    float getStopTime() const
    { return stopTime; }

    void setStartTime(float time)
    { startTime = time; }
    void setStopTime(float time)
    { stopTime = time; }

    const std::vector<Ogre::Vector3>& getInitialVertices() const
    { return initialVertices; }
    const std::vector<std::vector<Ogre::Vector3> >& getRelevantData() const
    { return relevantData; }
    const std::vector<std::vector<float> >& getRelevantTimes() const
    { return relevantTimes; }
    const std::vector<std::vector<Ogre::Vector3> >& getAdditionalVertices() const
    { return additionalVertices; }

    void read(NIFFile *nif)
    {
        int morphCount = nif->getInt();
        int vertCount  = nif->getInt();
        nif->getByte();
        int magic = nif->getInt();
        /*int type =*/ nif->getInt();

        for(int i = 0; i < vertCount; i++)
        {
            float x = nif->getFloat();
            float y = nif->getFloat();
            float z = nif->getFloat();
            initialVertices.push_back(Ogre::Vector3(x, y, z));
        }

        for(int i=1; i<morphCount; i++)
        {
            magic = nif->getInt();
            /*type =*/ nif->getInt();
            std::vector<Ogre::Vector3> current;
            std::vector<float> currentTime;
            for(int i = 0; i < magic; i++)
            {
                // Time, data, forward, backward tangents
                float time = nif->getFloat();
                float x = nif->getFloat();
                float y = nif->getFloat();
                float z = nif->getFloat();
                current.push_back(Ogre::Vector3(x,y,z));
                currentTime.push_back(time);
                //nif->getFloatLen(4*magic);
            }

            if(magic)
            {
                relevantData.push_back(current);
                relevantTimes.push_back(currentTime);
            }

            std::vector<Ogre::Vector3> verts;
            for(int i = 0; i < vertCount; i++)
            {
                float x = nif->getFloat();
                float y = nif->getFloat();
                float z = nif->getFloat();
                verts.push_back(Ogre::Vector3(x, y, z));
            }
            additionalVertices.push_back(verts);
        }
    }
};


class NiKeyframeData : public Record
{
    std::string bonename;
    //Rotations
    std::vector<Ogre::Quaternion> quats;
    std::vector<Ogre::Vector3> tbc;
    std::vector<float> rottime;
    float startTime;
    float stopTime;
    int rtype;

    //Translations
    std::vector<Ogre::Vector3> translist1;
    std::vector<Ogre::Vector3> translist2;
    std::vector<Ogre::Vector3> translist3;
    std::vector<Ogre::Vector3> transtbc;
    std::vector<float> transtime;
    int ttype;

    //Scalings
    std::vector<float> scalefactor;
    std::vector<float> scaletime;
    std::vector<float> forwards;
    std::vector<float> backwards;
    std::vector<Ogre::Vector3> tbcscale;
    int stype;

public:
    void clone(const NiKeyframeData &c)
    {
        quats = c.getQuat();
        tbc = c.getrTbc();
        rottime = c.getrTime();

        //types
        ttype = c.getTtype();
        rtype = c.getRtype();
        stype = c.getStype();


        translist1 = c.getTranslist1();
        translist2 = c.getTranslist2();
        translist3 = c.getTranslist3();

        transtime = c.gettTime();

        bonename = c.getBonename();
    }

    void setBonename(std::string bone)
    { bonename = bone; }
    void setStartTime(float start)
    { startTime = start; }
    void setStopTime(float end)
    { stopTime = end; }

    void read(NIFFile *nif)
    {
        // Rotations first
        int count = nif->getInt();
        //std::vector<Ogre::Quaternion> quat(count);
        //std::vector<float> rottime(count);
        if(count)
        {
            //TYPE1  LINEAR_KEY
            //TYPE2  QUADRATIC_KEY
            //TYPE3  TBC_KEY
            //TYPE4  XYZ_ROTATION_KEY
            //TYPE5  UNKNOWN_KEY
            rtype = nif->getInt();
            //std::cout << "Count: " << count << "Type: " << type << "\n";

            if(rtype == 1)
            {
                //We need to actually read in these values instead of skipping them
                //nif->skip(count*4*5); // time + quaternion
                for (int i = 0; i < count; i++)
                {
                    float time = nif->getFloat();
                    float w = nif->getFloat();
                    float x = nif->getFloat();
                    float y = nif->getFloat();
                    float z = nif->getFloat();
                    Ogre::Quaternion quat = Ogre::Quaternion(Ogre::Real(w), Ogre::Real(x), Ogre::Real(y), Ogre::Real(z));
                    quats.push_back(quat);
                    rottime.push_back(time);
                    //if(time == 0.0 || time > 355.5)
                    //    std::cout <<"Time:" << time << "W:" << w <<"X:" << x << "Y:" << y << "Z:" << z << "\n";
                }
            }
            else if(rtype == 3)
            {
                //Example - node 116 in base_anim.nif
                for (int i = 0; i < count; i++)
                {
                    float time = nif->getFloat();
                    float w = nif->getFloat();
                    float x = nif->getFloat();
                    float y = nif->getFloat();
                    float z = nif->getFloat();

                    float tbcx = nif->getFloat();
                    float tbcy = nif->getFloat();
                    float tbcz = nif->getFloat();

                    Ogre::Quaternion quat = Ogre::Quaternion(Ogre::Real(w), Ogre::Real(x), Ogre::Real(y), Ogre::Real(z));
                    Ogre::Vector3 vec = Ogre::Vector3(tbcx, tbcy, tbcz);
                    quats.push_back(quat);
                    rottime.push_back(time);
                    tbc.push_back(vec);
                    //if(time == 0.0 || time > 355.5)
                    //    std::cout <<"Time:" << time << "W:" << w <<"X:" << x << "Y:" << y << "Z:" << z << "\n";
                }
            }
            else if(rtype == 4)
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
                        else
                            nif->fail("Unknown sub-rotation type");
                    }
                }
            }
            else
                nif->fail("Unknown rotation type in NiKeyframeData");
        }
        //first = false;

        // Then translation
        count = nif->getInt();
        if(count)
        {
            ttype = nif->getInt();

            //std::cout << "TransCount:" << count << " Type: " << type << "\n";
            if(ttype == 1)
            {
                for(int i = 0; i < count; i++)
                {
                    float time = nif->getFloat();
                    float x = nif->getFloat();
                    float y = nif->getFloat();
                    float z = nif->getFloat();

                    Ogre::Vector3 trans = Ogre::Vector3(x, y, z);
                    translist1.push_back(trans);
                    transtime.push_back(time);
                }
                //nif->getFloatLen(count*4); // time + translation
            }
            else if(ttype == 2)
            {
                //Example - node 116 in base_anim.nif
                for(int i = 0; i < count; i++)
                {
                    float time = nif->getFloat();
                    float x = nif->getFloat();
                    float y = nif->getFloat();
                    float z = nif->getFloat();
                    float x2 = nif->getFloat();
                    float y2 = nif->getFloat();
                    float z2 = nif->getFloat();
                    float x3 = nif->getFloat();
                    float y3 = nif->getFloat();
                    float z3 = nif->getFloat();

                    Ogre::Vector3 trans = Ogre::Vector3(x, y, z);
                    Ogre::Vector3 trans2 = Ogre::Vector3(x2, y2, z2);
                    Ogre::Vector3 trans3 = Ogre::Vector3(x3, y3, z3);
                    transtime.push_back(time);
                    translist1.push_back(trans);
                    translist2.push_back(trans2);
                    translist3.push_back(trans3);
                }

                //nif->getFloatLen(count*10); // trans1 + forward + backward
            }
            else if(ttype == 3)
            {
                for(int i = 0; i < count; i++)
                {
                    float time = nif->getFloat();
                    float x = nif->getFloat();
                    float y = nif->getFloat();
                    float z = nif->getFloat();
                    float t = nif->getFloat();
                    float b = nif->getFloat();
                    float c = nif->getFloat();
                    Ogre::Vector3 trans = Ogre::Vector3(x, y, z);
                    Ogre::Vector3 tbc = Ogre::Vector3(t, b, c);
                    translist1.push_back(trans);
                    transtbc.push_back(tbc);
                    transtime.push_back(time);
                }
            //nif->getFloatLen(count*7); // trans1 + tension,bias,continuity
            }
            else nif->fail("Unknown translation type");
        }

        // Finally, scalings
        count = nif->getInt();
        if(count)
        {
            stype = nif->getInt();

            for(int i = 0; i < count; i++)
            {
                //int size = 0;
                if(stype >= 1 && stype < 4)
                {
                    float time = nif->getFloat();
                    float scale = nif->getFloat();
                    scaletime.push_back(time);
                    scalefactor.push_back(scale);
                    //size = 2; // time+scale
                }
                else
                    nif->fail("Unknown scaling type");

                if(stype == 2)
                {
                    //size = 4; // 1 + forward + backward (floats)
                    float forward = nif->getFloat();
                    float backward = nif->getFloat();
                    forwards.push_back(forward);
                    backwards.push_back(backward);
                }
                else if(stype == 3)
                {
                    //size = 5; // 1 + tbc
                    float tbcx = nif->getFloat();
                    float tbcy = nif->getFloat();
                    float tbcz = nif->getFloat();
                    Ogre::Vector3 vec = Ogre::Vector3(tbcx, tbcy, tbcz);
                    tbcscale.push_back(vec);
                }
            }
        }
        else
            stype = 0;
    }

    int getRtype() const
    { return rtype; }
    int getStype() const
    { return stype; }
    int getTtype() const
    { return ttype; }
    float getStartTime() const
    { return startTime; }
    float getStopTime() const
    { return stopTime; }
    const std::vector<Ogre::Quaternion>& getQuat() const
    { return quats; }
    const std::vector<Ogre::Vector3>& getrTbc() const
    { return tbc; }
    const std::vector<float>& getrTime() const
    { return rottime; }

    const std::vector<Ogre::Vector3>& getTranslist1() const
    { return translist1; }
    const std::vector<Ogre::Vector3>& getTranslist2() const
    { return translist2; }
    const std::vector<Ogre::Vector3>& getTranslist3() const
    { return translist3; }
    const std::vector<float>& gettTime() const
    { return transtime; }
    const std::vector<float>& getScalefactor() const
    { return scalefactor; }
    const std::vector<float>& getForwards() const
    { return forwards; }
    const std::vector<float>& getBackwards() const
    { return backwards; }
    const std::vector<Ogre::Vector3>& getScaleTbc() const
    { return tbcscale; }

    const std::vector<float>& getsTime() const
    { return scaletime; }
    const std::string& getBonename() const
    { return bonename; }
};

} // Namespace
#endif
