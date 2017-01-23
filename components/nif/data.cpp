#include "data.hpp"
#include "node.hpp"

#include <osg/Array>
#include <osg/PrimitiveSet>

namespace Nif
{
void NiSkinInstance::read(NIFStream *nif)
{
    data.read(nif);
    root.read(nif);
    bones.read(nif);
}

void NiSkinInstance::post(NIFFile *nif)
{
    data.post(nif);
    root.post(nif);
    bones.post(nif);

    if(data.empty() || root.empty())
        nif->fail("NiSkinInstance missing root or data");

    size_t bnum = bones.length();
    if(bnum != data->bones.size())
        nif->fail("Mismatch in NiSkinData bone count");

    root->makeRootBone(&data->trafo);

    for(size_t i=0; i<bnum; i++)
    {
        if(bones[i].empty())
            nif->fail("Oops: Missing bone! Don't know how to handle this.");
        bones[i]->makeBone(i, data->bones[i]);
    }
}

void ShapeData::read(NIFStream *nif)
{
    int verts = nif->getUShort();

    // Assign a VBO right off the bat to avoid race conditions later, in case two threads load this data into a Geometry at the same time
    osg::ref_ptr<osg::VertexBufferObject> vbo = new osg::VertexBufferObject;

    if(nif->getInt() && verts)
    {
        vertices = new osg::Vec3Array;
        nif->getVector3s(vertices, verts);
        vertices->setVertexBufferObject(vbo);
    }

    if(nif->getInt() && verts)
    {
        normals = new osg::Vec3Array(osg::Array::BIND_PER_VERTEX);
        nif->getVector3s(normals, verts);
        normals->setVertexBufferObject(vbo);
    }

    center = nif->getVector3();
    radius = nif->getFloat();

    if(nif->getInt() && verts)
    {
        colors = new osg::Vec4Array(osg::Array::BIND_PER_VERTEX);
        nif->getVector4s(colors, verts);
        colors->setVertexBufferObject(vbo);
    }

    // Only the first 6 bits are used as a count. I think the rest are
    // flags of some sort.
    int uvs = nif->getUShort();
    uvs &= 0x3f;

    if(nif->getInt() && verts)
    {
        uvlist.resize(uvs);
        for(int i = 0;i < uvs;i++)
        {
            osg::Vec2Array* list = uvlist[i] = new osg::Vec2Array(osg::Array::BIND_PER_VERTEX);
            list->setVertexBufferObject(vbo);
            nif->getVector2s(list, verts);

            // flip the texture coordinates to convert them to the OpenGL convention of bottom-left image origin
            for (unsigned int uv=0; uv<list->size(); ++uv)
            {
                (*list)[uv] = osg::Vec2((*list)[uv].x(), 1.f - (*list)[uv].y());
            }
        }
    }
}

void NiTriShapeData::read(NIFStream *nif)
{
    ShapeData::read(nif);

    /*int tris =*/ nif->getUShort();

    // We have three times as many vertices as triangles, so this
    // is always equal to tris*3.
    int cnt = nif->getInt();
    if (cnt)
    {
        triangles = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES);
        nif->getUShorts(triangles, cnt);

        // Assign an EBO right off the bat to avoid race conditions later, in case two threads load this data into a Geometry at the same time
        triangles->setElementBufferObject(new osg::ElementBufferObject);
    }

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

void NiAutoNormalParticlesData::read(NIFStream *nif)
{
    ShapeData::read(nif);

    // Should always match the number of vertices
    numParticles = nif->getUShort();

    particleRadius = nif->getFloat();
    activeCount = nif->getUShort();

    if(nif->getInt())
    {
        int numVerts = vertices->size();
        // Particle sizes
        nif->getFloats(sizes, numVerts);
    }
}

void NiRotatingParticlesData::read(NIFStream *nif)
{
    NiAutoNormalParticlesData::read(nif);

    if(nif->getInt())
    {
        int numVerts = vertices->size();
        // Rotation quaternions.
        nif->getQuaternions(rotations, numVerts);
    }
}

void NiPosData::read(NIFStream *nif)
{
    mKeyList.reset(new Vector3KeyMap);
    mKeyList->read(nif);
}

void NiUVData::read(NIFStream *nif)
{
    for(int i = 0;i < 4;i++)
    {
        mKeyList[i].reset(new FloatKeyMap);
        mKeyList[i]->read(nif);
    }
}

void NiFloatData::read(NIFStream *nif)
{
    mKeyList.reset(new FloatKeyMap);
    mKeyList->read(nif);
}

void NiPixelData::read(NIFStream *nif)
{
    fmt = (Format)nif->getUInt();

    rmask = nif->getInt(); // usually 0xff
    gmask = nif->getInt(); // usually 0xff00
    bmask = nif->getInt(); // usually 0xff0000
    amask = nif->getInt(); // usually 0xff000000 or zero

    bpp = nif->getInt();

    // Unknown
    nif->skip(12);

    mips = nif->getInt();

    // Bytes per pixel, should be bpp * 8
    /* int bytes = */ nif->getInt();

    for(int i=0; i<mips; i++)
    {
        // Image size and offset in the following data field
        Mipmap m;
        m.width = nif->getUInt();
        m.height = nif->getUInt();
        m.dataOffset = nif->getUInt();
        mipmaps.push_back(m);
    }

    // Read the data
    unsigned int dataSize = nif->getInt();
    data.reserve(dataSize);
    for (unsigned i=0; i<dataSize; ++i)
        data.push_back((unsigned char)nif->getChar());
}

void NiColorData::read(NIFStream *nif)
{
    mKeyMap.reset(new Vector4KeyMap);
    mKeyMap->read(nif);
}

void NiVisData::read(NIFStream *nif)
{
    int count = nif->getInt();
    mVis.resize(count);
    for(size_t i = 0;i < mVis.size();i++)
    {
        mVis[i].time = nif->getFloat();
        mVis[i].isSet = (nif->getChar() != 0);
    }
}

void NiSkinData::read(NIFStream *nif)
{
    trafo.rotation = nif->getMatrix3();
    trafo.pos = nif->getVector3();
    trafo.scale = nif->getFloat();

    int boneNum = nif->getInt();
    nif->getInt(); // -1

    bones.resize(boneNum);
    for(int i=0;i<boneNum;i++)
    {
        BoneInfo &bi = bones[i];

        bi.trafo.rotation = nif->getMatrix3();
        bi.trafo.pos = nif->getVector3();
        bi.trafo.scale = nif->getFloat();
        bi.boundSphereCenter = nif->getVector3();
        bi.boundSphereRadius = nif->getFloat();

        // Number of vertex weights
        bi.weights.resize(nif->getUShort());
        for(size_t j = 0;j < bi.weights.size();j++)
        {
            bi.weights[j].vertex = nif->getUShort();
            bi.weights[j].weight = nif->getFloat();
        }
    }
}

void NiMorphData::read(NIFStream *nif)
{
    int morphCount = nif->getInt();
    int vertCount  = nif->getInt();
    /*relative targets?*/nif->getChar();

    mMorphs.resize(morphCount);
    for(int i = 0;i < morphCount;i++)
    {
        mMorphs[i].mKeyFrames.reset(new FloatKeyMap);
        mMorphs[i].mKeyFrames->read(nif, true);
        mMorphs[i].mVertices = new osg::Vec3Array;
        nif->getVector3s(mMorphs[i].mVertices, vertCount);
    }
}

void NiKeyframeData::read(NIFStream *nif)
{
    mRotations.reset(new QuaternionKeyMap);
    mRotations->read(nif);
    if(mRotations->mInterpolationType == Vector3KeyMap::sXYZInterpolation)
    {
        //Chomp unused float
        nif->getFloat();
        mXRotations.reset(new FloatKeyMap);
        mYRotations.reset(new FloatKeyMap);
        mZRotations.reset(new FloatKeyMap);
        mXRotations->read(nif, true);
        mYRotations->read(nif, true);
        mZRotations->read(nif, true);
    }
    mTranslations.reset(new Vector3KeyMap);
    mTranslations->read(nif);
    mScales.reset(new FloatKeyMap);
    mScales->read(nif);
}

} // Namespace
