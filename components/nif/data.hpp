#ifndef OPENMW_COMPONENTS_NIF_DATA_HPP
#define OPENMW_COMPONENTS_NIF_DATA_HPP

#include "nifkey.hpp"
#include "niftypes.hpp" // NiTransform
#include "node.hpp"
#include "recordptr.hpp"

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
            uint32_t mWidth;
            uint32_t mHeight;
            uint32_t mOffset;

            void read(NIFStream* nif);
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
        // This is theoretically a "flat map" sorted by time
        std::shared_ptr<std::vector<std::pair<float, bool>>> mKeys;

        void read(NIFStream* nif) override;
    };

    struct NiSkinInstance : public Record
    {
        NiSkinDataPtr mData;
        NiSkinPartitionPtr mPartitions;
        NiAVObjectPtr mRoot;
        NiAVObjectList mBones;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        const Nif::NiSkinPartition* getPartitions() const;
    };

    struct BSDismemberSkinInstance : public NiSkinInstance
    {
        struct BodyPart
        {
            uint16_t mFlags;
            uint16_t mType;

            void read(NIFStream* nif);
        };

        std::vector<BodyPart> mParts;

        void read(NIFStream* nif) override;
    };

    struct BSSkinInstance : Record
    {
        NiAVObjectPtr mRoot;
        BSSkinBoneDataPtr mData;
        NiAVObjectList mBones;
        std::vector<osg::Vec3f> mScales;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiSkinData : public Record
    {
        using VertWeight = std::pair<unsigned short, float>;

        struct BoneInfo
        {
            NiTransform mTransform;
            osg::BoundingSpheref mBoundSphere;
            std::vector<VertWeight> mWeights;
        };

        NiTransform mTransform;
        std::vector<BoneInfo> mBones;
        NiSkinPartitionPtr mPartitions;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSSkinBoneData : Record
    {
        struct BoneInfo
        {
            osg::BoundingSpheref mBoundSphere;
            NiTransform mTransform;
        };

        std::vector<BoneInfo> mBones;

        void read(NIFStream* nif) override;
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
        BoolKeyMapPtr mKeyList;

        void read(NIFStream* nif) override;
    };

    struct NiBSplineData : public Record
    {
        std::vector<float> mFloatControlPoints;
        std::vector<int16_t> mCompactControlPoints;

        void read(NIFStream* nif) override;
    };

    struct NiBSplineBasisData : public Record
    {
        uint32_t mNumControlPoints;

        void read(NIFStream* nif) override;
    };

    struct NiAdditionalGeometryData : public Record
    {
        struct DataStream
        {
            uint32_t mType;
            uint32_t mUnitSize;
            uint32_t mTotalSize;
            uint32_t mStride;
            uint32_t mBlockIndex;
            uint32_t mBlockOffset;
            uint8_t mFlags;

            void read(NIFStream* nif);
        };

        struct DataBlock
        {
            bool mValid;
            uint32_t mBlockSize;
            std::vector<uint32_t> mBlockOffsets;
            std::vector<uint32_t> mDataSizes;
            std::vector<char> mData;
            uint32_t mShaderIndex;
            uint32_t mTotalSize;

            void read(NIFStream* nif, bool bsPacked);
        };

        uint16_t mNumVertices;
        std::vector<DataStream> mBlockInfos;
        std::vector<DataBlock> mBlocks;

        void read(NIFStream* nif);
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

    struct BSMultiBoundAABB : public BSMultiBoundData
    {
        osg::Vec3f mPosition;
        osg::Vec3f mExtents;

        void read(NIFStream* nif) override;
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

    struct BSAnimNote : public Record
    {
        enum class Type : uint32_t
        {
            Invalid = 0,
            GrabIK = 1,
            LookIK = 2,
        };

        Type mType;
        float mTime;
        uint32_t mArm;
        float mGain;
        uint32_t mState;

        void read(NIFStream* nif) override;
    };

    struct BSAnimNotes : public Record
    {
        BSAnimNoteList mList;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

}
#endif
