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

#include "nifkey.hpp"
#include "niftypes.hpp" // Transformation
#include "recordptr.hpp"
#include <components/nif/node.hpp>

namespace Nif
{

    // Common ancestor for several data classes
    struct NiGeometryData : public Record
    {
        // Interpretation of Flags field differs depending on the version
        enum DataFlags
        {
            DataFlag_HasUV = 0x0001,
            DataFlag_NumUVsMask = 0x003F,
            DataFlag_HasTangents = 0x1000,
        };

        int32_t mGroupId{ 0 };
        uint16_t mNumVertices;
        uint8_t mKeepFlags{ 0 };
        uint8_t mCompressFlags{ 0 };
        std::vector<osg::Vec3f> mVertices;
        uint16_t mDataFlags{ 0 };
        uint32_t mMaterialHash;
        std::vector<osg::Vec3f> mNormals, mTangents, mBitangents;
        osg::BoundingSpheref mBoundingSphere;
        std::vector<osg::Vec4f> mColors;
        std::vector<std::vector<osg::Vec2f>> mUVList;
        uint16_t mConsistencyType;

        void read(NIFStream* nif) override;
    };

    // Abstract
    struct NiTriBasedGeomData : public NiGeometryData
    {
        uint16_t mNumTriangles;

        void read(NIFStream* nif) override;
    };

    struct NiTriShapeData : public NiTriBasedGeomData
    {
        // Triangles, three vertex indices per triangle
        std::vector<unsigned short> mTriangles;
        std::vector<std::vector<unsigned short>> mMatchGroups;

        void read(NIFStream* nif) override;
    };

    struct NiTriStripsData : public NiTriBasedGeomData
    {
        // Triangle strips, series of vertex indices.
        std::vector<std::vector<unsigned short>> mStrips;

        void read(NIFStream* nif) override;
    };

    struct NiLinesData : public NiGeometryData
    {
        // Lines, series of indices that correspond to connected vertices.
        // NB: assumes <=65536 number of vertices
        std::vector<uint16_t> mLines;

        void read(NIFStream* nif) override;
    };

    struct NiParticlesData : public NiGeometryData
    {
        uint16_t mNumParticles{ 0 };
        uint16_t mActiveCount;

        std::vector<float> mRadii;
        std::vector<float> mSizes;
        std::vector<osg::Quat> mRotations;
        std::vector<float> mRotationAngles;
        std::vector<osg::Vec3f> mRotationAxes;

        bool mHasTextureIndices{ false };
        std::vector<osg::Vec4f> mSubtextureOffsets;
        float mAspectRatio{ 1.f };
        uint16_t mAspectFlags{ 0 };
        float mAspectRatio2;
        float mAspectSpeed, mAspectSpeed2;

        void read(NIFStream* nif) override;
    };

    struct NiRotatingParticlesData : public NiParticlesData
    {
        void read(NIFStream* nif) override;
    };

    struct NiPosData : public Record
    {
        Vector3KeyMapPtr mKeyList;

        void read(NIFStream* nif) override;
    };

    struct NiUVData : public Record
    {
        std::array<FloatKeyMapPtr, 4> mKeyList;

        void read(NIFStream* nif) override;
    };

    struct NiFloatData : public Record
    {
        FloatKeyMapPtr mKeyList;

        void read(NIFStream* nif) override;
    };

    struct NiPixelFormat
    {
        enum class Format : uint32_t
        {
            RGB = 0,
            RGBA = 1,
            Palette = 2,
            PaletteAlpha = 3,
            BGR = 4,
            BGRA = 5,
            DXT1 = 6,
            DXT3 = 7,
            DXT5 = 8,
        };

        struct ChannelData
        {
            enum class Type : uint32_t
            {
                Red = 0,
                Green = 1,
                Blue = 2,
                Alpha = 3,
                Compressed = 4,
                OffsetU = 5,
                OffsetV = 6,
                OffsetW = 7,
                OffsetQ = 8,
                Luma = 9,
                Height = 10,
                VectorX = 11,
                VectorY = 12,
                VectorZ = 13,
                Padding = 14,
                Intensity = 15,
                Index = 16,
                Depth = 17,
                Stencil = 18,
                Empty = 19,
            };

            enum class Convention : uint32_t
            {
                NormInt = 0,
                Half = 1,
                Float = 2,
                Index = 3,
                Compressed = 4,
                Unknown = 5,
                Int = 6,
            };

            Type mType;
            Convention mConvention;
            uint8_t mBitsPerChannel;
            bool mSigned;

            void read(NIFStream* nif);
        };

        Format mFormat{ Format::RGB };
        std::array<uint32_t, 4> mColorMasks;
        uint32_t mBitsPerPixel{ 0 };
        uint32_t mPixelTiling{ 0 };
        std::array<uint32_t, 2> mCompareBits;
        uint32_t mRendererHint{ 0 };
        uint32_t mExtraData{ 0 };
        uint8_t mFlags{ 0 };
        bool mUseSrgb{ false };
        std::array<ChannelData, 4> mChannels;

        void read(NIFStream* nif);
    };

    struct NiPixelData : public Record
    {
        struct Mipmap
        {
            uint32_t mWidth, mHeight;
            uint32_t mOffset;
        };

        NiPixelFormat mPixelFormat;
        NiPalettePtr mPalette;
        uint32_t mBytesPerPixel;
        std::vector<Mipmap> mMipmaps;
        uint32_t mNumFaces{ 1 };
        std::vector<uint8_t> mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiColorData : public Record
    {
        Vector4KeyMapPtr mKeyMap;

        void read(NIFStream* nif) override;
    };

    struct NiVisData : public Record
    {
        // TODO: investigate possible use of ByteKeyMap
        std::shared_ptr<std::map<float, bool>> mKeys;

        void read(NIFStream* nif) override;
    };

    struct NiSkinInstance : public Record
    {
        NiSkinDataPtr mData;
        NiSkinPartitionPtr mPartitions;
        NodePtr mRoot;
        NodeList mBones;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSDismemberSkinInstance : public NiSkinInstance
    {
        struct BodyPart
        {
            uint16_t mFlags;
            uint16_t mType;
        };

        std::vector<BodyPart> mParts;

        void read(NIFStream* nif) override;
    };

    struct NiSkinData : public Record
    {
        using VertWeight = std::pair<unsigned short, float>;

        struct BoneInfo
        {
            Transformation mTransform;
            osg::BoundingSpheref mBoundSphere;
            std::vector<VertWeight> mWeights;
        };

        Transformation mTransform;
        std::vector<BoneInfo> mBones;
        NiSkinPartitionPtr mPartitions;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiSkinPartition : public Record
    {
        struct Partition
        {
            std::vector<unsigned short> mBones;
            std::vector<unsigned short> mVertexMap;
            std::vector<float> mWeights;
            std::vector<std::vector<unsigned short>> mStrips;
            std::vector<unsigned short> mTriangles;
            std::vector<char> mBoneIndices;
            BSVertexDesc mVertexDesc;
            std::vector<unsigned short> mTrueTriangles;
            std::vector<std::vector<unsigned short>> mTrueStrips;
            uint8_t mLODLevel;
            bool mGlobalVB;

            void read(NIFStream* nif);
        };
        std::vector<Partition> mPartitions;

        unsigned int mDataSize;
        unsigned int mVertexSize;
        BSVertexDesc mVertexDesc;
        std::vector<BSVertexData> mVertexData;

        void read(NIFStream* nif) override;
    };

    struct NiMorphData : public Record
    {
        struct MorphData
        {
            FloatKeyMapPtr mKeyFrames;
            std::vector<osg::Vec3f> mVertices;
        };

        uint8_t mRelativeTargets;
        std::vector<MorphData> mMorphs;

        void read(NIFStream* nif) override;
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

        enum class AxisOrder : uint32_t
        {
            Order_XYZ = 0,
            Order_XZY = 1,
            Order_YZX = 2,
            Order_YXZ = 3,
            Order_ZXY = 4,
            Order_ZYX = 5,
            Order_XYX = 6,
            Order_YZY = 7,
            Order_ZXZ = 8
        };

        AxisOrder mAxisOrder{ AxisOrder::Order_XYZ };

        void read(NIFStream* nif) override;
    };

    struct NiPalette : public Record
    {
        // 32-bit RGBA colors that correspond to 8-bit indices
        std::vector<uint32_t> mColors;

        void read(NIFStream* nif) override;
    };

    struct NiStringPalette : public Record
    {
        std::string mPalette;

        void read(NIFStream* nif) override;
    };

    struct NiBoolData : public Record
    {
        ByteKeyMapPtr mKeyList;
        void read(NIFStream* nif) override;
    };

    struct BSMultiBound : public Record
    {
        BSMultiBoundDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // Abstract
    struct BSMultiBoundData : public Record
    {
    };

    struct BSMultiBoundOBB : public BSMultiBoundData
    {
        osg::Vec3f mCenter;
        osg::Vec3f mSize;
        Nif::Matrix3 mRotation;

        void read(NIFStream* nif) override;
    };

    struct BSMultiBoundSphere : public BSMultiBoundData
    {
        osg::BoundingSpheref mSphere;

        void read(NIFStream* nif) override;
    };

} // Namespace
#endif
