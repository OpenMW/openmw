#include "data.hpp"
#include "node.hpp"

namespace Nif
{
void NiSkinInstance::read(NIFStream *nif)
{
    data.read(nif);
    if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,101))
        partitions.read(nif);
    root.read(nif);
    bones.read(nif);
}

void NiSkinInstance::post(NIFFile *nif)
{
    data.post(nif);
    partitions.post(nif);
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

void NiGeometryData::read(NIFStream *nif)
{
    if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,114))
        nif->getInt(); // Group ID. (Almost?) always 0.

    int verts = nif->getUShort();

    if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
        nif->skip(2); // Keep flags and compress flags

    if (nif->getBoolean())
        nif->getVector3s(vertices, verts);

    unsigned int dataFlags = 0;
    if (nif->getVersion() >= NIFStream::generateVersion(10,0,1,0))
        dataFlags = nif->getUShort();

    if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
        nif->getUInt(); // Material CRC

    if (nif->getBoolean())
    {
        nif->getVector3s(normals, verts);
        if (dataFlags & 0x1000)
        {
            nif->getVector3s(tangents, verts);
            nif->getVector3s(bitangents, verts);
        }
    }

    center = nif->getVector3();
    radius = nif->getFloat();

    if (nif->getBoolean())
        nif->getVector4s(colors, verts);

    unsigned int numUVs = dataFlags;
    if (nif->getVersion() <= NIFStream::generateVersion(4,2,2,0))
        numUVs = nif->getUShort();

    // In Morrowind this field only corresponds to the number of UV sets.
    // In later games only the first 6 bits are used as a count and the rest are flags.
    if (nif->getVersion() > NIFFile::NIFVersion::VER_MW)
    {
        numUVs &= 0x3f;
        if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() > 0)
            numUVs &= 0x1;
    }

    bool hasUVs = true;
    if (nif->getVersion() <= NIFFile::NIFVersion::VER_MW)
        hasUVs = nif->getBoolean();
    if (hasUVs)
    {
        uvlist.resize(numUVs);
        for (unsigned int i = 0; i < numUVs; i++)
        {
            nif->getVector2s(uvlist[i], verts);
            // flip the texture coordinates to convert them to the OpenGL convention of bottom-left image origin
            for (unsigned int uv=0; uv<uvlist[i].size(); ++uv)
            {
                uvlist[i][uv] = osg::Vec2f(uvlist[i][uv].x(), 1.f - uvlist[i][uv].y());
            }
        }
    }

    if (nif->getVersion() >= NIFStream::generateVersion(10,0,1,0))
        nif->getUShort(); // Consistency flags

    if (nif->getVersion() >= NIFStream::generateVersion(20,0,0,4))
        nif->skip(4); // Additional data
}

void NiTriShapeData::read(NIFStream *nif)
{
    NiGeometryData::read(nif);

    /*int tris =*/ nif->getUShort();

    // We have three times as many vertices as triangles, so this
    // is always equal to tris*3.
    int cnt = nif->getInt();
    bool hasTriangles = true;
    if (nif->getVersion() > NIFFile::NIFVersion::VER_OB_OLD)
        hasTriangles = nif->getBoolean();
    if (hasTriangles)
        nif->getUShorts(triangles, cnt);

    // Read the match list, which lists the vertices that are equal to
    // vertices. We don't actually need need this for anything, so
    // just skip it.
    unsigned short verts = nif->getUShort();
    for (unsigned short i=0; i < verts; i++)
    {
        // Number of vertices matching vertex 'i'
        int num = nif->getUShort();
        nif->skip(num * sizeof(short));
    }
}

void NiTriStripsData::read(NIFStream *nif)
{
    NiGeometryData::read(nif);

    // Every strip with n points defines n-2 triangles, so this should be unnecessary.
    /*int tris =*/ nif->getUShort();
    // Number of triangle strips
    int numStrips = nif->getUShort();

    std::vector<unsigned short> lengths;
    nif->getUShorts(lengths, numStrips);

    // "Has Strips" flag. Exceptionally useful.
    bool hasStrips = true;
    if (nif->getVersion() > NIFFile::NIFVersion::VER_OB_OLD)
        hasStrips = nif->getBoolean();
    if (!hasStrips || !numStrips)
        return;

    strips.resize(numStrips);
    for (int i = 0; i < numStrips; i++)
        nif->getUShorts(strips[i], lengths[i]);
}

void NiLinesData::read(NIFStream *nif)
{
    NiGeometryData::read(nif);
    size_t num = vertices.size();
    std::vector<char> flags;
    nif->getChars(flags, num);
    // Can't construct a line from a single vertex.
    if (num < 2)
        return;
    // Convert connectivity flags into usable geometry. The last element needs special handling.
    for (size_t i = 0; i < num-1; ++i)
    {
        if (flags[i] & 1)
        {
            lines.emplace_back(i);
            lines.emplace_back(i+1);
        }
    }
    // If there are just two vertices, they can be connected twice. Probably isn't critical.
    if (flags[num-1] & 1)
    {
        lines.emplace_back(num-1);
        lines.emplace_back(0);
    }
}

void NiParticlesData::read(NIFStream *nif)
{
    NiGeometryData::read(nif);

    // Should always match the number of vertices
    if (nif->getVersion() <= NIFFile::NIFVersion::VER_MW)
        numParticles = nif->getUShort();

    if (nif->getVersion() <= NIFStream::generateVersion(10,0,1,0))
        std::fill(particleRadii.begin(), particleRadii.end(), nif->getFloat());
    else if (nif->getBoolean())
        nif->getFloats(particleRadii, vertices.size());
    activeCount = nif->getUShort();

    // Particle sizes
    if (nif->getBoolean())
        nif->getFloats(sizes, vertices.size());

    if (nif->getVersion() >= NIFStream::generateVersion(10,0,1,0) && nif->getBoolean())
        nif->getQuaternions(rotations, vertices.size());
    if (nif->getVersion() >= NIFStream::generateVersion(20,0,0,4))
    {
        if (nif->getBoolean())
            nif->getFloats(rotationAngles, vertices.size());
        if (nif->getBoolean())
            nif->getVector3s(rotationAxes, vertices.size());
    }

}

void NiRotatingParticlesData::read(NIFStream *nif)
{
    NiParticlesData::read(nif);

    if (nif->getVersion() <= NIFStream::generateVersion(4,2,2,0) && nif->getBoolean())
        nif->getQuaternions(rotations, vertices.size());
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

    if (nif->getVersion() < NIFStream::generateVersion(10,4,0,2))
    {
        for (unsigned int i = 0; i < 4; ++i)
            colorMask[i] = nif->getUInt();
        bpp = nif->getUInt();
        nif->skip(8); // "Old Fast Compare". Whatever that means.
        if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
            pixelTiling = nif->getUInt();
    }
    else // TODO: see if anything from here needs to be implemented
    {
        bpp = nif->getChar();
        nif->skip(4); // Renderer hint
        nif->skip(4); // Extra data
        nif->skip(4); // Flags
        pixelTiling = nif->getUInt();
        if (nif->getVersion() >= NIFStream::generateVersion(20,3,0,4))
            sRGB = nif->getBoolean();
        nif->skip(4*10); // Channel data
    }

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
    unsigned int numPixels = nif->getUInt();
    bool hasFaces = nif->getVersion() >= NIFStream::generateVersion(10,4,0,2);
    unsigned int numFaces = hasFaces ? nif->getUInt() : 1;
    if (numPixels && numFaces)
        nif->getUChars(data, numPixels * numFaces);
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
    if (nif->getVersion() >= NIFFile::NIFVersion::VER_MW && nif->getVersion() <= NIFStream::generateVersion(10,1,0,0))
        partitions.read(nif);

    // Has vertex weights flag
    if (nif->getVersion() > NIFStream::generateVersion(4,2,1,0) && !nif->getBoolean())
        return;

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

void NiSkinData::post(NIFFile *nif)
{
    partitions.post(nif);
}

void NiSkinPartition::read(NIFStream *nif)
{
    unsigned int num = nif->getUInt();
    data.resize(num);
    for (auto& partition : data)
        partition.read(nif);
}

void NiSkinPartition::Partition::read(NIFStream *nif)
{
    size_t numVertices = nif->getUShort();
    size_t numTriangles = nif->getUShort();
    size_t numBones = nif->getUShort();
    size_t numStrips = nif->getUShort();
    size_t bonesPerVertex = nif->getUShort();
    if (numBones)
        nif->getUShorts(bones, numBones);

    bool hasVertexMap = true;
    if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
        hasVertexMap = nif->getBoolean();
    if (hasVertexMap && numVertices)
        nif->getUShorts(vertexMap, numVertices);

    bool hasVertexWeights = true;
    if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
        hasVertexWeights = nif->getBoolean();
    if (hasVertexWeights && numVertices && bonesPerVertex)
        nif->getFloats(weights, numVertices * bonesPerVertex);

    std::vector<unsigned short> stripLengths;
    if (numStrips)
        nif->getUShorts(stripLengths, numStrips);

    bool hasFaces = true;
    if (nif->getVersion() >= NIFStream::generateVersion(10,1,0,0))
        hasFaces = nif->getBoolean();
    if (hasFaces)
    {
        if (numStrips)
        {
            strips.resize(numStrips);
            for (size_t i = 0; i < numStrips; i++)
                nif->getUShorts(strips[i], stripLengths[i]);
        }
        else if (numTriangles)
            nif->getUShorts(triangles, numTriangles * 3);
    }
    bool hasBoneIndices = nif->getChar() != 0;
    if (hasBoneIndices && numVertices && bonesPerVertex)
        nif->getChars(boneIndices, numVertices * bonesPerVertex);
    if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
    {
        nif->getChar(); // LOD level
        nif->getBoolean(); // Global VB
    }
}

void NiMorphData::read(NIFStream *nif)
{
    int morphCount = nif->getInt();
    int vertCount  = nif->getInt();
    nif->getChar(); // Relative targets, always 1

    mMorphs.resize(morphCount);
    for(int i = 0;i < morphCount;i++)
    {
        mMorphs[i].mKeyFrames = std::make_shared<FloatKeyMap>();
        mMorphs[i].mKeyFrames->read(nif, true, /*morph*/true);
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
        if (nif->getVersion() <= NIFStream::generateVersion(10,1,0,0))
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

void NiStringPalette::read(NIFStream *nif)
{
    palette = nif->getString();
    if (nif->getUInt() != palette.size())
        nif->file->warn("Failed size check in NiStringPalette");
}

void NiBoolData::read(NIFStream *nif)
{
    mKeyList = std::make_shared<ByteKeyMap>();
    mKeyList->read(nif);
}

} // Namespace
