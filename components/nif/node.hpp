#ifndef OPENMW_COMPONENTS_NIF_NODE_HPP
#define OPENMW_COMPONENTS_NIF_NODE_HPP

#include <array>
#include <unordered_map>

#include <osg/Plane>

#include "base.hpp"

class btCollisionShape;

namespace Nif
{

    struct NiNode;

    struct BoundingVolume
    {
        enum Type : uint32_t
        {
            BASE_BV = 0xFFFFFFFF,
            SPHERE_BV = 0,
            BOX_BV = 1,
            CAPSULE_BV = 2,
            LOZENGE_BV = 3,
            UNION_BV = 4,
            HALFSPACE_BV = 5
        };

        struct NiBoxBV
        {
            osg::Vec3f mCenter;
            Matrix3 mAxes;
            osg::Vec3f mExtents;
        };

        struct NiCapsuleBV
        {
            osg::Vec3f mCenter, mAxis;
            float mExtent{ 0.f }, mRadius{ 0.f };
        };

        struct NiLozengeBV
        {
            float mRadius{ 0.f }, mExtent0{ 0.f }, mExtent1{ 0.f };
            osg::Vec3f mCenter, mAxis0, mAxis1;
        };

        struct NiHalfSpaceBV
        {
            osg::Plane mPlane;
            osg::Vec3f mOrigin;
        };

        uint32_t mType{ BASE_BV };
        osg::BoundingSpheref mSphere;
        NiBoxBV mBox;
        NiCapsuleBV mCapsule;
        NiLozengeBV mLozenge;
        std::vector<BoundingVolume> mChildren;
        NiHalfSpaceBV mHalfSpace;

        void read(NIFStream* nif);
    };

    struct NiSequenceStreamHelper : NiObjectNET
    {
    };

    // NiAVObject is an object that is a part of the main NIF tree. It has
    // a parent node (unless it's the root) and transformation relative to its parent.
    struct NiAVObject : NiObjectNET
    {
        enum Flags
        {
            Flag_Hidden = 0x0001,
            Flag_MeshCollision = 0x0002,
            Flag_BBoxCollision = 0x0004,
            Flag_ActiveCollision = 0x0020
        };

        // Node flags. Interpretation depends on the record type.
        uint32_t mFlags;
        NiTransform mTransform;
        osg::Vec3f mVelocity;
        NiPropertyList mProperties;
        BoundingVolume mBounds;
        NiCollisionObjectPtr mCollision;
        // Parent nodes for the node. Only types derived from NiNode can be parents.
        std::vector<NiNode*> mParents;
        bool mIsBone{ false };

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        void setBone();
        bool isHidden() const { return mFlags & Flag_Hidden; }
        bool hasMeshCollision() const { return mFlags & Flag_MeshCollision; }
        bool hasBBoxCollision() const { return mFlags & Flag_BBoxCollision; }
        bool collisionActive() const { return mFlags & Flag_ActiveCollision; }
    };

    struct NiNode : NiAVObject
    {
        enum BSAnimFlags
        {
            AnimFlag_AutoPlay = 0x0020
        };

        enum BSParticleFlags
        {
            ParticleFlag_AutoPlay = 0x0020,
            ParticleFlag_LocalSpace = 0x0080
        };

        NiAVObjectList mChildren;
        NiAVObjectList mEffects;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiGeometry : NiAVObject
    {
        /* Possible flags:
            0x40 - mesh has no vertex normals ?

            Only flags included in 0x47 (ie. 0x01, 0x02, 0x04 and 0x40) have
            been observed so far.
        */

        struct MaterialData
        {
            std::vector<std::string> mNames;
            std::vector<int> mExtra;
            int32_t mActive{ -1 };
            bool mNeedsUpdate{ false };

            void read(NIFStream* nif);
        };

        NiGeometryDataPtr mData;
        NiSkinInstancePtr mSkin;
        MaterialData mMaterial;
        BSShaderPropertyPtr mShaderProperty;
        NiAlphaPropertyPtr mAlphaProperty;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        virtual std::unique_ptr<btCollisionShape> getCollisionShape() const
        {
            throw std::runtime_error("NiGeometry::getCollisionShape() called on base class");
        }
    };

    // Abstract triangle-based geometry
    struct NiTriBasedGeom : NiGeometry
    {
    };

    struct NiTriShape : NiTriBasedGeom
    {
        std::unique_ptr<btCollisionShape> getCollisionShape() const override;
    };

    struct BSSegmentedTriShape : NiTriShape
    {
        struct SegmentData
        {
            uint8_t mFlags;
            uint32_t mStartIndex;
            uint32_t mNumTriangles;

            void read(NIFStream* nif);
        };

        std::vector<SegmentData> mSegments;

        void read(NIFStream* nif);
    };

    struct NiTriStrips : NiTriBasedGeom
    {
        std::unique_ptr<btCollisionShape> getCollisionShape() const override;
    };

    struct NiLines : NiTriBasedGeom
    {
        std::unique_ptr<btCollisionShape> getCollisionShape() const override;
    };

    struct NiParticles : NiGeometry
    {
        std::unique_ptr<btCollisionShape> getCollisionShape() const override;
    };

    struct BSLODTriShape : NiTriShape
    {
        std::array<uint32_t, 3> mLOD;
        void read(NIFStream* nif) override;
    };

    struct NiCamera : NiAVObject
    {
        uint16_t mCameraFlags{ 0 };
        // Camera frustum
        float mLeft, mRight, mTop, mBottom, mNearDist, mFarDist;
        bool mOrthographic{ false };
        // Viewport
        float mVLeft, mVRight, mVTop, mVBottom;
        float mLODAdjust;
        NiAVObjectPtr mScene;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // A node used as the base to switch between child nodes, such as for LOD levels.
    struct NiSwitchNode : NiNode
    {
        uint16_t mSwitchFlags;
        uint32_t mInitialIndex;

        void read(NIFStream* nif) override;
    };

    struct NiLODNode : NiSwitchNode
    {
        struct LODRange
        {
            float mMinRange;
            float mMaxRange;

            void read(NIFStream* nif);
        };

        osg::Vec3f mLODCenter;
        std::vector<LODRange> mLODLevels;

        void read(NIFStream* nif) override;
    };

    struct NiFltAnimationNode : NiSwitchNode
    {
        enum Flags
        {
            Flag_Swing = 0x40
        };

        float mDuration;

        void read(NIFStream* nif) override;

        bool swing() const { return mFlags & Flag_Swing; }
    };

    // Abstract
    struct NiAccumulator : Record
    {
        void read(NIFStream* nif) override {}
    };

    // Node children sorters
    struct NiClusterAccumulator : NiAccumulator
    {
    };

    struct NiAlphaAccumulator : NiClusterAccumulator
    {
    };

    struct NiSortAdjustNode : NiNode
    {
        enum class SortingMode : uint32_t
        {
            Inherit,
            Off,
            Subsort,
        };

        SortingMode mMode;
        NiAccumulatorPtr mSubSorter;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiBillboardNode : NiNode
    {
        int mMode;

        void read(NIFStream* nif) override;
    };

    struct NiDefaultAVObjectPalette : Record
    {
        NiAVObjectPtr mScene;
        std::unordered_map<std::string, NiAVObjectPtr> mObjects;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSTreeNode : NiNode
    {
        NiAVObjectList mBones1, mBones2;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSMultiBoundNode : NiNode
    {
        enum class BSCPCullingType : uint32_t
        {
            Normal,
            AllPass,
            AllFail,
            IgnoreMultiBounds,
            ForceMultiBoundsNoUpdate,
        };

        BSMultiBoundPtr mMultiBound;
        BSCPCullingType mCullingType;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSVertexDesc
    {
        uint8_t mVertexDataSize;
        uint8_t mDynamicVertexSize;
        uint8_t mUV1Offset;
        uint8_t mUV2Offset;
        uint8_t mNormalOffset;
        uint8_t mTangentOffset;
        uint8_t mColorOffset;
        uint8_t mSkinningDataOffset;
        uint8_t mLandscapeDataOffset;
        uint8_t mEyeDataOffset;
        uint16_t mFlags;

        enum VertexAttribute
        {
            Vertex = 0x0001,
            UVs = 0x0002,
            UVs_2 = 0x0004,
            Normals = 0x0008,
            Tangents = 0x0010,
            Vertex_Colors = 0x0020,
            Skinned = 0x0040,
            Land_Data = 0x0080,
            Eye_Data = 0x0100,
            Instance = 0x0200,
            Full_Precision = 0x0400,
        };

        void read(NIFStream* nif);
    };

    struct BSVertexData
    {
        osg::Vec4f mVertex; // Bitangent X is stored in the fourth component
        std::array<Misc::float16_t, 4> mHalfVertex; // Ditto
        std::array<Misc::float16_t, 2> mUV;
        std::array<char, 4> mNormal; // Bitangent Y is stored in the fourth component
        std::array<char, 4> mTangent; // Bitangent Z is stored in the fourth component
        std::array<char, 4> mVertColor;
        std::array<Misc::float16_t, 4> mBoneWeights;
        std::array<char, 4> mBoneIndices;
        float mEyeData;

        void read(NIFStream* nif, uint16_t flags);
    };

    struct BSTriShape : NiAVObject
    {
        osg::BoundingSpheref mBoundingSphere;
        std::array<float, 6> mBoundMinMax;
        RecordPtrT<Record> mSkin;
        BSShaderPropertyPtr mShaderProperty;
        NiAlphaPropertyPtr mAlphaProperty;
        BSVertexDesc mVertDesc;
        uint32_t mDataSize;
        std::vector<BSVertexData> mVertData;
        std::vector<unsigned short> mTriangles;
        uint32_t mParticleDataSize;
        std::vector<Misc::float16_t> mParticleVerts;
        std::vector<Misc::float16_t> mParticleNormals;
        std::vector<unsigned short> mParticleTriangles;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSDynamicTriShape : BSTriShape
    {
        uint32_t mDynamicDataSize;
        std::vector<osg::Vec4f> mDynamicData;

        void read(NIFStream* nif) override;
    };

    struct BSMeshLODTriShape : BSTriShape
    {
        std::array<uint32_t, 3> mLOD;

        void read(NIFStream* nif) override;
    };

    struct BSSubIndexTriShape : BSTriShape
    {
        struct SubSegment
        {
            uint32_t mStartIndex;
            uint32_t mNumPrimitives;
            uint32_t mArrayIndex;

            void read(NIFStream* nif);
        };

        struct Segment
        {
            uint32_t mStartIndex;
            uint32_t mNumPrimitives;
            uint32_t mParentArrayIndex;
            std::vector<SubSegment> mSubSegments;

            void read(NIFStream* nif);
        };

        struct SubSegmentDataRecord
        {
            uint32_t mUserSlotID;
            uint32_t mMaterial;
            std::vector<float> mExtraData;

            void read(NIFStream* nif);
        };

        struct SubSegmentData
        {
            std::vector<uint32_t> mArrayIndices;
            std::vector<SubSegmentDataRecord> mDataRecords;
            std::string mSSFFile;

            void read(NIFStream* nif);
        };

        struct Segmentation
        {
            uint32_t mNumPrimitives;
            uint32_t mNumTotalSegments;
            std::vector<Segment> mSegments;
            SubSegmentData mSubSegmentData;

            void read(NIFStream* nif);
        };

        std::vector<BSSegmentedTriShape::SegmentData> mSegments; // SSE
        Segmentation mSegmentation; // FO4

        void read(NIFStream* nif) override;
    };

    struct BSValueNode : NiNode
    {
        enum Flags
        {
            Flag_BillboardWorldZ = 0x1,
            Flag_UsePlayerAdjust = 0x2,
        };

        uint32_t mValue;
        uint8_t mValueFlags;

        void read(NIFStream* nif) override;
    };

    struct BSOrderedNode : NiNode
    {
        osg::Vec4f mAlphaSortBound;
        bool mStaticBound;

        void read(NIFStream* nif) override;
    };

    struct BSRangeNode : NiNode
    {
        uint8_t mMin, mMax;
        uint8_t mCurrent;

        void read(NIFStream* nif) override;
    };

    struct BSResourceID
    {
        uint32_t mFileHash;
        std::array<char, 4> mExtension;
        uint32_t mDirectoryHash;

        void read(NIFStream* nif);
    };

    struct BSDistantObjectInstance
    {
        BSResourceID mResourceID;
        std::vector<osg::Matrixf> mTransforms;

        void read(NIFStream* nif);
    };

    struct BSShaderTextureArray
    {
        std::vector<std::vector<std::string>> mTextureArrays;

        void read(NIFStream* nif);
    };

    struct BSDistantObjectInstancedNode : BSMultiBoundNode
    {
        std::vector<BSDistantObjectInstance> mInstances;
        std::array<BSShaderTextureArray, 3> mShaderTextureArrays;

        void read(NIFStream* nif) override;
    };

}
#endif
