#include "data.hpp"

#include <components/debug/debuglog.hpp>

#include "exception.hpp"
#include "nifkey.hpp"
#include "node.hpp"

namespace Nif
{
    void NiGeometryData::read(NIFStream* nif)
    {
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 114))
            nif->getInt(); // Group ID. (Almost?) always 0.

        int verts = nif->getUShort();

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->skip(2); // Keep flags and compress flags

        if (nif->getBoolean())
            nif->readVector(vertices, verts);

        unsigned int dataFlags = 0;
        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
            dataFlags = nif->getUShort();

        if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS
            && nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
            nif->getUInt(); // Material CRC

        if (nif->getBoolean())
        {
            nif->readVector(normals, verts);
            if (dataFlags & 0x1000)
            {
                nif->readVector(tangents, verts);
                nif->readVector(bitangents, verts);
            }
        }

        center = nif->getVector3();
        radius = nif->getFloat();

        if (nif->getBoolean())
            nif->readVector(colors, verts);

        unsigned int numUVs = dataFlags;
        if (nif->getVersion() <= NIFStream::generateVersion(4, 2, 2, 0))
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
                nif->readVector(uvlist[i], verts);
                // flip the texture coordinates to convert them to the OpenGL convention of bottom-left image origin
                for (unsigned int uv = 0; uv < uvlist[i].size(); ++uv)
                {
                    uvlist[i][uv] = osg::Vec2f(uvlist[i][uv].x(), 1.f - uvlist[i][uv].y());
                }
            }
        }

        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
            nif->getUShort(); // Consistency flags

        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 4))
            nif->skip(4); // Additional data
    }

    void NiTriBasedGeomData::read(NIFStream* nif)
    {
        NiGeometryData::read(nif);

        nif->read(mNumTriangles);
    }

    void NiTriShapeData::read(NIFStream* nif)
    {
        NiTriBasedGeomData::read(nif);

        // We have three times as many vertices as triangles, so this
        // is always equal to mNumTriangles * 3.
        int cnt = nif->getInt();
        bool hasTriangles = true;
        if (nif->getVersion() > NIFFile::NIFVersion::VER_OB_OLD)
            hasTriangles = nif->getBoolean();
        if (hasTriangles)
            nif->readVector(triangles, cnt);

        // Read the match list, which lists the vertices that are equal to
        // vertices. We don't actually need need this for anything, so
        // just skip it.
        unsigned short verts = nif->getUShort();
        for (unsigned short i = 0; i < verts; i++)
        {
            // Number of vertices matching vertex 'i'
            int num = nif->getUShort();
            nif->skip(num * sizeof(short));
        }
    }

    void NiTriStripsData::read(NIFStream* nif)
    {
        NiTriBasedGeomData::read(nif);

        uint16_t numStrips;
        nif->read(numStrips);
        std::vector<uint16_t> lengths;
        nif->readVector(lengths, numStrips);

        bool hasStrips = true;
        if (nif->getVersion() > NIFFile::NIFVersion::VER_OB_OLD)
            nif->read(hasStrips);
        if (!hasStrips || !numStrips)
            return;

        mStrips.resize(numStrips);
        for (int i = 0; i < numStrips; i++)
            nif->readVector(mStrips[i], lengths[i]);
    }

    void NiLinesData::read(NIFStream* nif)
    {
        NiGeometryData::read(nif);

        size_t num = vertices.size();
        std::vector<uint8_t> flags;
        nif->readVector(flags, num);
        // Can't construct a line from a single vertex.
        if (num < 2)
            return;
        // Convert connectivity flags into usable geometry. The last element needs special handling.
        for (size_t i = 0; i < num - 1; ++i)
        {
            if (flags[i] & 1)
            {
                mLines.emplace_back(i);
                mLines.emplace_back(i + 1);
            }
        }
        // If there are just two vertices, they can be connected twice. Probably isn't critical.
        if (flags[num - 1] & 1)
        {
            mLines.emplace_back(num - 1);
            mLines.emplace_back(0);
        }
    }

    void NiParticlesData::read(NIFStream* nif)
    {
        NiGeometryData::read(nif);

        // Should always match the number of vertices
        if (nif->getVersion() <= NIFFile::NIFVersion::VER_MW)
            numParticles = nif->getUShort();

        if (nif->getVersion() <= NIFStream::generateVersion(10, 0, 1, 0))
            std::fill(particleRadii.begin(), particleRadii.end(), nif->getFloat());
        else if (nif->getBoolean())
            nif->readVector(particleRadii, vertices.size());
        activeCount = nif->getUShort();

        // Particle sizes
        if (nif->getBoolean())
            nif->readVector(sizes, vertices.size());

        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0) && nif->getBoolean())
            nif->readVector(rotations, vertices.size());
        if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 4))
        {
            if (nif->getBoolean())
                nif->readVector(rotationAngles, vertices.size());
            if (nif->getBoolean())
                nif->readVector(rotationAxes, vertices.size());
        }
    }

    void NiRotatingParticlesData::read(NIFStream* nif)
    {
        NiParticlesData::read(nif);

        if (nif->getVersion() > NIFStream::generateVersion(4, 2, 2, 0))
            return;

        bool hasRotations;
        nif->read(hasRotations);
        if (hasRotations)
            nif->readVector(rotations, vertices.size());
    }

    void NiPosData::read(NIFStream* nif)
    {
        mKeyList = std::make_shared<Vector3KeyMap>();
        mKeyList->read(nif);
    }

    void NiUVData::read(NIFStream* nif)
    {
        for (FloatKeyMapPtr& keys : mKeyList)
        {
            keys = std::make_shared<FloatKeyMap>();
            keys->read(nif);
        }
    }

    void NiFloatData::read(NIFStream* nif)
    {
        mKeyList = std::make_shared<FloatKeyMap>();
        mKeyList->read(nif);
    }

    void NiPixelFormat::read(NIFStream* nif)
    {
        mFormat = static_cast<Format>(nif->get<uint32_t>());
        if (nif->getVersion() <= NIFStream::generateVersion(10, 4, 0, 1))
        {
            nif->readArray(mColorMasks);
            nif->read(mBitsPerPixel);
            nif->readArray(mCompareBits);
            if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
                nif->read(mPixelTiling);
        }
        else
        {
            mBitsPerPixel = nif->get<uint8_t>();
            nif->read(mRendererHint);
            nif->read(mExtraData);
            nif->read(mFlags);
            nif->read(mPixelTiling);
            if (nif->getVersion() >= NIFStream::generateVersion(20, 3, 0, 4))
                nif->read(mUseSrgb);
            for (int i = 0; i < 4; i++)
                mChannels[i].read(nif);
        }
    }

    void NiPixelFormat::ChannelData::read(NIFStream* nif)
    {
        mType = static_cast<Type>(nif->get<uint32_t>());
        mConvention = static_cast<Convention>(nif->get<uint32_t>());
        nif->read(mBitsPerChannel);
        nif->read(mSigned);
    }

    void NiPixelData::read(NIFStream* nif)
    {
        mPixelFormat.read(nif);
        mPalette.read(nif);
        mMipmaps.resize(nif->get<uint32_t>());
        nif->read(mBytesPerPixel);
        for (Mipmap& mip : mMipmaps)
        {
            nif->read(mip.mWidth);
            nif->read(mip.mHeight);
            nif->read(mip.mOffset);
        }
        uint32_t numPixels;
        nif->read(numPixels);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 4, 0, 2))
            nif->read(mNumFaces);
        nif->readVector(mData, numPixels * mNumFaces);
    }

    void NiPixelData::post(Reader& nif)
    {
        mPalette.post(nif);
    }

    void NiColorData::read(NIFStream* nif)
    {
        mKeyMap = std::make_shared<Vector4KeyMap>();
        mKeyMap->read(nif);
    }

    void NiVisData::read(NIFStream* nif)
    {
        mKeys = std::make_shared<std::map<float, bool>>();
        uint32_t numKeys;
        nif->read(numKeys);
        for (size_t i = 0; i < numKeys; i++)
        {
            float time;
            char value;
            nif->read(time);
            nif->read(value);
            (*mKeys)[time] = (value != 0);
        }
    }

    void NiSkinInstance::read(NIFStream* nif)
    {
        mData.read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 101))
            mPartitions.read(nif);
        mRoot.read(nif);
        readRecordList(nif, mBones);
    }

    void NiSkinInstance::post(Reader& nif)
    {
        mData.post(nif);
        mPartitions.post(nif);
        mRoot.post(nif);
        postRecordList(nif, mBones);

        if (mData.empty() || mRoot.empty())
            throw Nif::Exception("NiSkinInstance missing root or data", nif.getFilename());

        if (mBones.size() != mData->bones.size())
            throw Nif::Exception("Mismatch in NiSkinData bone count", nif.getFilename());

        for (auto& bone : mBones)
        {
            if (bone.empty())
                throw Nif::Exception("Oops: Missing bone! Don't know how to handle this.", nif.getFilename());
            bone->setBone();
        }
    }

    void BSDismemberSkinInstance::read(NIFStream* nif)
    {
        NiSkinInstance::read(nif);

        mParts.resize(nif->get<uint32_t>());
        for (BodyPart& part : mParts)
        {
            nif->read(part.mFlags);
            nif->read(part.mType);
        }
    }

    void NiSkinData::read(NIFStream* nif)
    {
        trafo.rotation = nif->getMatrix3();
        trafo.pos = nif->getVector3();
        trafo.scale = nif->getFloat();

        int boneNum = nif->getInt();
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_MW
            && nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 0))
            partitions.read(nif);

        bool hasVertexWeights = true;
        if (nif->getVersion() > NIFStream::generateVersion(4, 2, 1, 0))
            hasVertexWeights = nif->getBoolean();

        bones.resize(boneNum);
        for (BoneInfo& bi : bones)
        {
            bi.trafo.rotation = nif->getMatrix3();
            bi.trafo.pos = nif->getVector3();
            bi.trafo.scale = nif->getFloat();
            bi.boundSphereCenter = nif->getVector3();
            bi.boundSphereRadius = nif->getFloat();

            size_t numVertices = nif->getUShort();

            if (!hasVertexWeights)
                continue;

            bi.weights.resize(numVertices);
            for (size_t j = 0; j < bi.weights.size(); j++)
            {
                bi.weights[j].vertex = nif->getUShort();
                bi.weights[j].weight = nif->getFloat();
            }
        }
    }

    void NiSkinData::post(Reader& nif)
    {
        partitions.post(nif);
    }

    void NiSkinPartition::read(NIFStream* nif)
    {
        nif->read(mPartitionNum);
        mPartitions.resize(mPartitionNum);

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_SSE)
        {
            nif->read(mDataSize);
            nif->read(mVertexSize);
            mVertexDesc.read(nif);

            mVertexData.resize(mDataSize / mVertexSize);
            for (auto& vertexData : mVertexData)
                vertexData.read(nif, mVertexDesc.mFlags);
        }

        for (auto& partition : mPartitions)
            partition.read(nif);
    }

    void NiSkinPartition::Partition::read(NIFStream* nif)
    {
        size_t numVertices = nif->getUShort();
        size_t numTriangles = nif->getUShort();
        size_t numBones = nif->getUShort();
        size_t numStrips = nif->getUShort();
        size_t bonesPerVertex = nif->getUShort();
        nif->readVector(bones, numBones);

        bool hasVertexMap = true;
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            hasVertexMap = nif->getBoolean();
        if (hasVertexMap)
            nif->readVector(vertexMap, numVertices);

        bool hasVertexWeights = true;
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            hasVertexWeights = nif->getBoolean();
        if (hasVertexWeights)
            nif->readVector(weights, numVertices * bonesPerVertex);

        std::vector<unsigned short> stripLengths;
        nif->readVector(stripLengths, numStrips);

        bool hasFaces = true;
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            hasFaces = nif->getBoolean();
        if (hasFaces)
        {
            if (numStrips)
            {
                strips.resize(numStrips);
                for (size_t i = 0; i < numStrips; i++)
                    nif->readVector(strips[i], stripLengths[i]);
            }
            else
                nif->readVector(triangles, numTriangles * 3);
        }
        bool hasBoneIndices = nif->getChar() != 0;
        if (hasBoneIndices)
            nif->readVector(boneIndices, numVertices * bonesPerVertex);
        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
        {
            nif->getChar(); // LOD level
            nif->getBoolean(); // Global VB
        }

        if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_SSE)
        {
            mVertexDesc.read(nif);
            nif->readVector(trueTriangles, numTriangles * 3);
        }
    }

    std::vector<unsigned short> NiSkinPartition::Partition::getTrueTriangles() const
    {
        if (!trueTriangles.empty())
            return trueTriangles;

        std::vector<unsigned short> remappedTriangles;
        if (vertexMap.empty() || triangles.empty())
            return remappedTriangles;

        remappedTriangles = triangles;

        for (unsigned short& index : remappedTriangles)
            index = vertexMap[index];
        return remappedTriangles;
    }

    std::vector<std::vector<unsigned short>> NiSkinPartition::Partition::getTrueStrips() const
    {
        if (!trueTriangles.empty())
            return {};

        std::vector<std::vector<unsigned short>> remappedStrips;
        if (vertexMap.empty() || strips.empty())
            return remappedStrips;

        remappedStrips = strips;
        for (auto& strip : remappedStrips)
            for (auto& index : strip)
                index = vertexMap[index];

        return remappedStrips;
    }

    void NiMorphData::read(NIFStream* nif)
    {
        uint32_t numMorphs, numVerts;
        nif->read(numMorphs);
        nif->read(numVerts);
        nif->read(mRelativeTargets);

        mMorphs.resize(numMorphs);
        for (MorphData& morph : mMorphs)
        {
            morph.mKeyFrames = std::make_shared<FloatKeyMap>();
            morph.mKeyFrames->read(nif, /*morph*/ true);
            nif->readVector(morph.mVertices, numVerts);
        }
    }

    void NiKeyframeData::read(NIFStream* nif)
    {
        mRotations = std::make_shared<QuaternionKeyMap>();
        mRotations->read(nif);
        if (mRotations->mInterpolationType == InterpolationType_XYZ)
        {
            if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 0))
                mAxisOrder = static_cast<AxisOrder>(nif->getInt());
            mXRotations = std::make_shared<FloatKeyMap>();
            mYRotations = std::make_shared<FloatKeyMap>();
            mZRotations = std::make_shared<FloatKeyMap>();
            mXRotations->read(nif);
            mYRotations->read(nif);
            mZRotations->read(nif);
        }
        mTranslations = std::make_shared<Vector3KeyMap>();
        mTranslations->read(nif);
        mScales = std::make_shared<FloatKeyMap>();
        mScales->read(nif);
    }

    void NiPalette::read(NIFStream* nif)
    {
        bool useAlpha = nif->get<uint8_t>() != 0;
        uint32_t alphaMask = useAlpha ? 0 : 0xFF000000;

        uint32_t numEntries;
        nif->read(numEntries);

        // Fill the entire palette with black even if there isn't enough entries.
        mColors.resize(256);
        if (numEntries > 256)
            mColors.resize(numEntries);

        for (uint32_t i = 0; i < numEntries; i++)
        {
            nif->read(mColors[i]);
            mColors[i] |= alphaMask;
        }
    }

    void NiStringPalette::read(NIFStream* nif)
    {
        mPalette = nif->getStringPalette();
        if (nif->get<uint32_t>() != mPalette.size())
            Log(Debug::Warning) << "NIFFile Warning: Failed size check in NiStringPalette. File: "
                                << nif->getFile().getFilename();
    }

    void NiBoolData::read(NIFStream* nif)
    {
        mKeyList = std::make_shared<ByteKeyMap>();
        mKeyList->read(nif);
    }

    void BSMultiBound::read(NIFStream* nif)
    {
        mData.read(nif);
    }

    void BSMultiBound::post(Reader& nif)
    {
        mData.post(nif);
    }

    void BSMultiBoundOBB::read(NIFStream* nif)
    {
        nif->read(mCenter);
        nif->read(mSize);
        nif->read(mRotation);
    }

    void BSMultiBoundSphere::read(NIFStream* nif)
    {
        nif->read(mCenter);
        nif->read(mRadius);
    }

} // Namespace
