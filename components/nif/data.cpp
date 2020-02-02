#include "data.hpp"
#include "node.hpp"

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

    for(size_t i=0; i<bnum; i++)
    {
        if(bones[i].empty())
            nif->fail("Oops: Missing bone! Don't know how to handle this.");
        bones[i]->setBone();
    }
}

void ShapeData::read(NIFStream *nif)
{
    int verts = nif->getUShort();

    if (nif->getBoolean())
        nif->getVector3s(vertices, verts);

    if (nif->getBoolean())
        nif->getVector3s(normals, verts);

    center = nif->getVector3();
    radius = nif->getFloat();

    if (nif->getBoolean())
        nif->getVector4s(colors, verts);

    // Only the first 6 bits are used as a count. I think the rest are
    // flags of some sort.
    int uvs = nif->getUShort();
    uvs &= 0x3f;

    if(nif->getInt())
    {
        uvlist.resize(uvs);
        for(int i = 0;i < uvs;i++)
        {
            nif->getVector2s(uvlist[i], verts);
            // flip the texture coordinates to convert them to the OpenGL convention of bottom-left image origin
            for (unsigned int uv=0; uv<uvlist[i].size(); ++uv)
            {
                uvlist[i][uv] = osg::Vec2f(uvlist[i][uv].x(), 1.f - uvlist[i][uv].y());
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
    nif->getUShorts(triangles, cnt);

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

void NiTriStripsData::read(NIFStream *nif)
{
    ShapeData::read(nif);

    // Every strip with n points defines n-2 triangles, so this should be unnecessary.
    /*int tris =*/ nif->getUShort();
    // Number of triangle strips
    int numStrips = nif->getUShort();

    std::vector<unsigned short> lengths;
    nif->getUShorts(lengths, numStrips);

    if (!numStrips)
        return;

    strips.resize(numStrips);
    for (int i = 0; i < numStrips; i++)
        nif->getUShorts(strips[i], lengths[i]);
}

void NiAutoNormalParticlesData::read(NIFStream *nif)
{
    ShapeData::read(nif);

    // Should always match the number of vertices
    numParticles = nif->getUShort();

    particleRadius = nif->getFloat();
    activeCount = nif->getUShort();

    if (nif->getBoolean())
    {
        // Particle sizes
        nif->getFloats(sizes, vertices.size());
    }
}

void NiRotatingParticlesData::read(NIFStream *nif)
{
    NiAutoNormalParticlesData::read(nif);

    if (nif->getBoolean())
    {
        // Rotation quaternions.
        nif->getQuaternions(rotations, vertices.size());
    }
}

void NiPosData::read(NIFStream *nif)
{
    mKeyList = std::make_shared<Vector3KeyMap>();
    mKeyList->read(nif);
}

void NiUVData::read(NIFStream *nif)
{
    for(int i = 0;i < 4;i++)
    {
        mKeyList[i] = std::make_shared<FloatKeyMap>();
        mKeyList[i]->read(nif);
    }
}

void NiFloatData::read(NIFStream *nif)
{
    mKeyList = std::make_shared<FloatKeyMap>();
    mKeyList->read(nif);
}

void NiPixelData::read(NIFStream *nif)
{
    fmt = (Format)nif->getUInt();

    rmask = nif->getUInt(); // usually 0xff
    gmask = nif->getUInt(); // usually 0xff00
    bmask = nif->getUInt(); // usually 0xff0000
    amask = nif->getUInt(); // usually 0xff000000 or zero

    bpp = nif->getUInt();

    // 8 bytes of "Old Fast Compare". Whatever that means.
    nif->skip(8);
    palette.read(nif);

    numberOfMipmaps = nif->getUInt();

    // Bytes per pixel, should be bpp / 8
    /* int bytes = */ nif->getUInt();

    for(unsigned int i=0; i<numberOfMipmaps; i++)
    {
        // Image size and offset in the following data field
        Mipmap m;
        m.width = nif->getUInt();
        m.height = nif->getUInt();
        m.dataOffset = nif->getUInt();
        mipmaps.push_back(m);
    }

    // Read the data
    unsigned int dataSize = nif->getUInt();
    data.reserve(dataSize);
    for (unsigned i=0; i<dataSize; ++i)
        data.push_back((unsigned char)nif->getChar());
}

void NiPixelData::post(NIFFile *nif)
{
    palette.post(nif);
}

void NiColorData::read(NIFStream *nif)
{
    mKeyMap = std::make_shared<Vector4KeyMap>();
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
    if (nif->getVersion() >= NIFFile::NIFVersion::VER_MW && nif->getVersion() <= NIFFile::NIFVersion::VER_GAMEBRYO)
        nif->skip(4); // NiSkinPartition link

    bones.resize(boneNum);
    for (BoneInfo &bi : bones)
    {
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
        mMorphs[i].mKeyFrames = std::make_shared<FloatKeyMap>();
        mMorphs[i].mKeyFrames->read(nif, true);
        nif->getVector3s(mMorphs[i].mVertices, vertCount);
    }
}

void NiKeyframeData::read(NIFStream *nif)
{
    mRotations = std::make_shared<QuaternionKeyMap>();
    mRotations->read(nif);
    if(mRotations->mInterpolationType == InterpolationType_XYZ)
    {
        //Chomp unused float
        nif->getFloat();
        mXRotations = std::make_shared<FloatKeyMap>();
        mYRotations = std::make_shared<FloatKeyMap>();
        mZRotations = std::make_shared<FloatKeyMap>();
        mXRotations->read(nif, true);
        mYRotations->read(nif, true);
        mZRotations->read(nif, true);
    }
    mTranslations = std::make_shared<Vector3KeyMap>();
    mTranslations->read(nif);
    mScales = std::make_shared<FloatKeyMap>();
    mScales->read(nif);
}

void NiPalette::read(NIFStream *nif)
{
    unsigned int alphaMask = !nif->getChar() ? 0xFF000000 : 0;
    // Fill the entire palette with black even if there isn't enough entries.
    colors.resize(256);
    unsigned int numEntries = nif->getUInt();
    for (unsigned int i = 0; i < numEntries; i++)
        colors[i] = nif->getUInt() | alphaMask;
}

} // Namespace
