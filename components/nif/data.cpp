#include "data.hpp"

#include <components/debug/debuglog.hpp>

#include "exception.hpp"
#include "nifkey.hpp"
#include "node.hpp"

namespace Nif
{
    namespace
    {
        void readNiSkinDataVertWeight(NIFStream& stream, NiSkinData::VertWeight& value)
        {
            auto& [vertex, weight] = value;
            stream.read(vertex);
            stream.read(weight);
        }

        struct ReadNiSkinDataBoneInfo
        {
            bool mHasVertexWeights;

            void operator()(NIFStream& stream, NiSkinData::BoneInfo& value) const
            {
                stream.read(value.mTransform);
                stream.read(value.mBoundSphere);

                const uint16_t numVertices = stream.get<uint16_t>();

                if (!mHasVertexWeights)
                    return;

                stream.readVectorOfRecords(numVertices, readNiSkinDataVertWeight, value.mWeights);
            }
        };

        struct ReadNiMorphDataMorphData
        {
            uint32_t mNumVerts;

            void operator()(NIFStream& stream, NiMorphData::MorphData& value) const
            {
                value.mKeyFrames = std::make_shared<FloatKeyMap>();
                value.mKeyFrames->read(&stream, /*morph*/ true);
                stream.readVector(value.mVertices, mNumVerts);
            }
        };
    }

    void NiGeometryData::read(NIFStream* nif)
    {
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 114))
            nif->read(mGroupId);

        nif->read(mNumVertices);

        bool isPSysData = false;
        switch (recType)
        {
            case RC_NiPSysData:
            case RC_NiMeshPSysData:
            case RC_BSStripPSysData:
                isPSysData = true;
                break;
            default:
                break;
        }
        bool hasData = !isPSysData || nif->getBethVersion() < NIFFile::BethVersion::BETHVER_FO3;

        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
        {
            nif->read(mKeepFlags);
            nif->read(mCompressFlags);
        }

        if (nif->get<bool>() && hasData)
            nif->readVector(mVertices, mNumVertices);

        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
        {
            nif->read(mDataFlags);

            if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS
                && nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
                nif->read(mMaterialHash);
        }

        if (nif->get<bool>() && hasData)
        {
            nif->readVector(mNormals, mNumVertices);
            if (mDataFlags & DataFlag_HasTangents)
            {
                nif->readVector(mTangents, mNumVertices);
                nif->readVector(mBitangents, mNumVertices);
            }
        }

        nif->read(mBoundingSphere);

        if (nif->get<bool>() && hasData)
            nif->readVector(mColors, mNumVertices);

        if (nif->getVersion() <= NIFStream::generateVersion(4, 2, 2, 0))
            nif->read(mDataFlags);

        // In 4.0.0.2 the flags field corresponds to the number of UV sets.
        // In later revisions the part that corresponds to the number is narrower.
        uint16_t numUVs = mDataFlags;
        if (nif->getVersion() > NIFFile::NIFVersion::VER_MW)
        {
            numUVs &= DataFlag_NumUVsMask;
            if (nif->getVersion() == NIFFile::NIFVersion::VER_BGS && nif->getBethVersion() > 0)
                numUVs &= DataFlag_HasUV;
        }
        else if (!nif->get<bool>())
            numUVs = 0;

        if (hasData)
        {
            mUVList.resize(numUVs);
            for (std::vector<osg::Vec2f>& list : mUVList)
            {
                nif->readVector(list, mNumVertices);
                // flip the texture coordinates to convert them to the OpenGL convention of bottom-left image origin
                for (osg::Vec2f& uv : list)
                    uv.y() = 1.f - uv.y();
            }
        }

        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
        {
            nif->read(mConsistencyType);
            if (nif->getVersion() >= NIFStream::generateVersion(20, 0, 0, 4))
                nif->skip(4); // Additional data
        }
    }

    void NiTriBasedGeomData::read(NIFStream* nif)
    {
        NiGeometryData::read(nif);

        nif->read(mNumTriangles);
    }

    void NiTriShapeData::read(NIFStream* nif)
    {
        NiTriBasedGeomData::read(nif);

        uint32_t numIndices;
        nif->read(numIndices);
        if (nif->getVersion() > NIFFile::NIFVersion::VER_OB_OLD && !nif->get<bool>())
            numIndices = 0;
        nif->readVector(mTriangles, numIndices);
        mMatchGroups.resize(nif->get<uint16_t>());
        for (auto& group : mMatchGroups)
            nif->readVector(group, nif->get<uint16_t>());
    }

    void NiTriStripsData::read(NIFStream* nif)
    {
        NiTriBasedGeomData::read(nif);

        uint16_t numStrips;
        nif->read(numStrips);
        std::vector<uint16_t> lengths;
        nif->readVector(lengths, numStrips);
        if (nif->getVersion() > NIFFile::NIFVersion::VER_OB_OLD && !nif->get<bool>())
            numStrips = 0;
        mStrips.resize(numStrips);
        for (int i = 0; i < numStrips; i++)
            nif->readVector(mStrips[i], lengths[i]);
    }

    void NiLinesData::read(NIFStream* nif)
    {
        NiGeometryData::read(nif);

        std::vector<uint8_t> flags;
        nif->readVector(flags, mNumVertices);
        // Can't construct a line from a single vertex.
        if (mNumVertices < 2)
            return;
        // There can't be more than 2 indices for each vertex
        mLines.reserve(mNumVertices * 2);
        // Convert connectivity flags into usable geometry. The last element needs special handling.
        for (uint16_t i = 0; i < mNumVertices - 1; ++i)
        {
            if (flags[i] & 1)
            {
                mLines.emplace_back(i);
                mLines.emplace_back(i + 1);
            }
        }
        // If there are just two vertices, they can be connected twice. Probably isn't critical.
        if (flags[mNumVertices - 1] & 1)
        {
            mLines.emplace_back(mNumVertices - 1);
            mLines.emplace_back(0);
        }
        mLines.shrink_to_fit();
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

    void NiPixelData::Mipmap::read(NIFStream* nif)
    {
        nif->read(mWidth);
        nif->read(mHeight);
        nif->read(mOffset);
    }

    void NiPixelData::read(NIFStream* nif)
    {
        mPixelFormat.read(nif);
        mPalette.read(nif);
        const uint32_t mipmapsCount = nif->get<uint32_t>();
        nif->read(mBytesPerPixel);
        nif->readVectorOfRecords(mipmapsCount, mMipmaps);
        const uint32_t numPixels = nif->get<uint32_t>();
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
        const auto readPair = [](NIFStream& stream, std::pair<float, bool>& value) {
            stream.read(value.first);
            value.second = stream.get<uint8_t>() != 0;
        };

        std::vector<std::pair<float, bool>> keys;
        nif->readVectorOfRecords<uint32_t>(readPair, keys);
        mKeys = std::make_shared<std::vector<std::pair<float, bool>>>(std::move(keys));
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

        if (mBones.size() != mData->mBones.size())
            throw Nif::Exception("Mismatch in NiSkinData bone count", nif.getFilename());

        for (auto& bone : mBones)
        {
            if (bone.empty())
                throw Nif::Exception("Oops: Missing bone! Don't know how to handle this.", nif.getFilename());
            bone->setBone();
        }
    }

    const Nif::NiSkinPartition* NiSkinInstance::getPartitions() const
    {
        const Nif::NiSkinPartition* partitions = nullptr;
        if (!mPartitions.empty())
            partitions = mPartitions.getPtr();
        else if (!mData.empty() && !mData->mPartitions.empty())
            partitions = mData->mPartitions.getPtr();

        return partitions;
    }

    void BSDismemberSkinInstance::BodyPart::read(NIFStream* nif)
    {
        nif->read(mFlags);
        nif->read(mType);
    }

    void BSDismemberSkinInstance::read(NIFStream* nif)
    {
        NiSkinInstance::read(nif);

        nif->readVectorOfRecords<uint32_t>(mParts);
    }

    void BSSkinInstance::read(NIFStream* nif)
    {
        mRoot.read(nif);
        mData.read(nif);
        readRecordList(nif, mBones);
        nif->readVector(mScales, nif->get<uint32_t>());
    }

    void BSSkinInstance::post(Reader& nif)
    {
        mRoot.post(nif);
        mData.post(nif);
        postRecordList(nif, mBones);
        if (mData.empty() || mRoot.empty())
            throw Nif::Exception("BSSkin::Instance missing root or data", nif.getFilename());

        if (mBones.size() != mData->mBones.size())
            throw Nif::Exception("Mismatch in BSSkin::BoneData bone count", nif.getFilename());

        for (auto& bone : mBones)
        {
            if (bone.empty())
                throw Nif::Exception("Oops: Missing bone! Don't know how to handle this.", nif.getFilename());
            bone->setBone();
        }
    }

    void NiSkinData::read(NIFStream* nif)
    {
        nif->read(mTransform);

        const uint32_t numBones = nif->get<uint32_t>();
        bool hasVertexWeights = true;
        if (nif->getVersion() >= NIFFile::NIFVersion::VER_MW)
        {
            if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 0))
                mPartitions.read(nif);

            if (nif->getVersion() >= NIFStream::generateVersion(4, 2, 1, 0))
                nif->read(hasVertexWeights);
        }

        const ReadNiSkinDataBoneInfo readBoneInfo{ .mHasVertexWeights = hasVertexWeights };
        nif->readVectorOfRecords(numBones, readBoneInfo, mBones);
    }

    void NiSkinData::post(Reader& nif)
    {
        mPartitions.post(nif);
    }

    void BSSkinBoneData::read(NIFStream* nif)
    {
        mBones.resize(nif->get<uint32_t>());
        for (BoneInfo& bone : mBones)
        {
            nif->read(bone.mBoundSphere);
            nif->read(bone.mTransform);
        }
    }

    void NiSkinPartition::read(NIFStream* nif)
    {
        mPartitions.resize(nif->get<uint32_t>());

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
        uint16_t numVertices, numTriangles, numBones, numStrips, bonesPerVertex;
        nif->read(numVertices);
        nif->read(numTriangles);
        nif->read(numBones);
        nif->read(numStrips);
        nif->read(bonesPerVertex);
        nif->readVector(mBones, numBones);
        bool hasPresenceFlags = nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0);
        if (!hasPresenceFlags || nif->get<bool>())
            nif->readVector(mVertexMap, numVertices);
        if (!hasPresenceFlags || nif->get<bool>())
            nif->readVector(mWeights, static_cast<size_t>(numVertices) * bonesPerVertex);
        std::vector<unsigned short> stripLengths;
        nif->readVector(stripLengths, numStrips);
        if (!hasPresenceFlags || nif->get<bool>())
        {
            if (numStrips)
            {
                mStrips.resize(numStrips);
                for (size_t i = 0; i < numStrips; i++)
                    nif->readVector(mStrips[i], stripLengths[i]);
            }
            else
                nif->readVector(mTriangles, numTriangles * 3);
        }
        if (nif->get<uint8_t>() != 0)
            nif->readVector(mBoneIndices, static_cast<size_t>(numVertices) * bonesPerVertex);
        if (nif->getBethVersion() > NIFFile::BethVersion::BETHVER_FO3)
        {
            nif->read(mLODLevel);
            nif->read(mGlobalVB);
            if (nif->getBethVersion() == NIFFile::BethVersion::BETHVER_SSE)
            {
                mVertexDesc.read(nif);
                nif->readVector(mTrueTriangles, numTriangles * 3);
            }
        }

        // Not technically a part of the loading process
        if (mTrueTriangles.empty() && !mVertexMap.empty())
        {
            if (!mStrips.empty())
            {
                mTrueStrips = mStrips;
                for (auto& strip : mTrueStrips)
                    for (auto& index : strip)
                        index = mVertexMap[index];
            }
            else if (!mTriangles.empty())
            {
                mTrueTriangles = mTriangles;
                for (unsigned short& index : mTrueTriangles)
                    index = mVertexMap[index];
            }
        }
    }

    void NiMorphData::read(NIFStream* nif)
    {
        const uint32_t numMorphs = nif->get<uint32_t>();
        const uint32_t numVerts = nif->get<uint32_t>();
        nif->read(mRelativeTargets);

        const ReadNiMorphDataMorphData readMorph{ .mNumVerts = numVerts };
        nif->readVectorOfRecords(numMorphs, readMorph, mMorphs);
    }

    void NiKeyframeData::read(NIFStream* nif)
    {
        mRotations = std::make_shared<QuaternionKeyMap>();
        mRotations->read(nif);
        if (mRotations->mInterpolationType == InterpolationType_XYZ)
        {
            if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 0))
                mAxisOrder = static_cast<AxisOrder>(nif->get<uint32_t>());
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
        mColors.resize(numEntries > 256 ? numEntries : 256);
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
        mKeyList = std::make_shared<BoolKeyMap>();
        mKeyList->read(nif);
    }

    void NiBSplineData::read(NIFStream* nif)
    {
        nif->readVector(mFloatControlPoints, nif->get<uint32_t>());
        nif->readVector(mCompactControlPoints, nif->get<uint32_t>());
    }

    void NiBSplineBasisData::read(NIFStream* nif)
    {
        nif->read(mNumControlPoints);
    }

    void NiAdditionalGeometryData::read(NIFStream* nif)
    {
        nif->read(mNumVertices);
        mBlockInfos.resize(nif->get<uint32_t>());
        for (DataStream& info : mBlockInfos)
            info.read(nif);
        mBlocks.resize(nif->get<uint32_t>());
        for (DataBlock& block : mBlocks)
            block.read(nif, recType == RC_BSPackedAdditionalGeometryData);
    }

    void NiAdditionalGeometryData::DataStream::read(NIFStream* nif)
    {
        nif->read(mType);
        nif->read(mUnitSize);
        nif->read(mTotalSize);
        nif->read(mStride);
        nif->read(mBlockIndex);
        nif->read(mBlockOffset);
        nif->read(mFlags);
    }

    void NiAdditionalGeometryData::DataBlock::read(NIFStream* nif, bool bsPacked)
    {
        nif->read(mValid);
        if (!mValid)
            return;
        nif->read(mBlockSize);
        nif->readVector(mBlockOffsets, nif->get<uint32_t>());
        nif->readVector(mDataSizes, nif->get<uint32_t>());
        nif->readVector(mData, mDataSizes.size() * mBlockSize);
        if (bsPacked)
        {
            nif->read(mShaderIndex);
            nif->read(mTotalSize);
        }
    }

    void BSMultiBound::read(NIFStream* nif)
    {
        mData.read(nif);
    }

    void BSMultiBound::post(Reader& nif)
    {
        mData.post(nif);
    }

    void BSMultiBoundAABB::read(NIFStream* nif)
    {
        nif->read(mPosition);
        nif->read(mExtents);
    }

    void BSMultiBoundOBB::read(NIFStream* nif)
    {
        nif->read(mCenter);
        nif->read(mSize);
        nif->read(mRotation);
    }

    void BSMultiBoundSphere::read(NIFStream* nif)
    {
        nif->read(mSphere);
    }

    void BSAnimNote::read(NIFStream* nif)
    {
        mType = static_cast<Type>(nif->get<uint32_t>());
        nif->read(mTime);
        if (mType == Type::GrabIK)
        {
            nif->read(mArm);
        }
        else if (mType == Type::LookIK)
        {
            nif->read(mGain);
            nif->read(mState);
        }
    }

    void BSAnimNotes::read(NIFStream* nif)
    {
        mList.resize(nif->get<uint16_t>());
        for (auto& note : mList)
            note.read(nif);
    }

    void BSAnimNotes::post(Reader& nif)
    {
        postRecordList(nif, mList);
    }

} // Namespace
