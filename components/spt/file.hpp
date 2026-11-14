#ifndef OPENMW_COMPONENTS_SPT_FILE_HPP
#define OPENMW_COMPONENTS_SPT_FILE_HPP

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/files/istreamptr.hpp>

namespace Spt
{
    class SPTStream;

    struct ControlPoint
    {
        float mPosX{};
        float mPosY{};
        float mTanX{};
        float mTanY{};
        float mTanLen{};
    };

    struct Curve
    {
        float mMin{};
        float mMax{};
        float mVariance{};
        std::vector<ControlPoint> mPoints;

        float evaluate(float t) const;
    };

    struct SPTLevelData
    {
        Curve mDisturbance;
        Curve mGravity;
        Curve mFlexibility;
        Curve mFlexibilityProfile;
        Curve mLength;
        Curve mRadius;
        Curve mRadiusProfile;
        Curve mStartAngle;
        Curve mGravityProfile;

        int32_t mSegmentsCrossSection{};
        int32_t mSegmentsLength{};
        float mGenerationFirst{};
        float mGenerationLast{};
        float mGenerationFrequency{};
        float mTextureTilingU{};
        float mTextureTilingV{};
        int8_t mTextureTilingUAbs{};
        int8_t mTextureTilingVAbs{};

        void read(SPTStream& stream);
    };

    struct SPTLeafTextureMap
    {
        int8_t mMakeBlossom{};
        osg::Vec3f mColor;
        float mOrientationVariance{};
        std::string mFilename;
        osg::Vec3f mOrigin;
        osg::Vec3f mSize;
        osg::Vec3f mUnknown4006;
        float mUnknown4007{};
    };

    struct SPTLeafData
    {
        float mBlossomDistance{};
        int32_t mBlossomDepth{};
        float mBlossomWeight{};
        int8_t mUnknown3003{};
        float mUnknown3004{};
        float mUnknown3005{};
        int8_t mUnknown3006{};
        float mPlacementTolerance{};
        int32_t mPlacementCollision{};
        int8_t mLightingDimmingToggle{};
        float mLightingDimming{};

        int32_t mNumLeafTextures{};
        std::vector<SPTLeafTextureMap> mLeafTextures;

        void read(SPTStream& stream);
    };

    struct SPTLightingMaterial
    {
        osg::Vec3f mDiffuse;
        osg::Vec3f mAmbient;
        osg::Vec3f mSpecular;
        osg::Vec3f mEmissive;
        float mShine{};

        void read(SPTStream& stream);
    };

    struct SPTLightingData
    {
        int32_t mBranchStyle{};
        SPTLightingMaterial mBranchMaterial;
        int32_t mLeafStyle{};
        SPTLightingMaterial mLeafMaterial;
        float mLeafLightingAdjust{};
        int32_t mLeafStaticShadows{};
        int32_t mFrondStyle{};
        SPTLightingMaterial mFrondMaterial;

        void read(SPTStream& stream);
    };

    struct SPTLodBranchesData
    {
        int32_t mBranchNumLods{};
        float mBranchMinVol{};
        float mLeafReduction{};
        int32_t mLeafNumLods{};
        float mBranchMaxVol{};
        float mBranchFuzziness{};
        float mBranchRetention{};

        void read(SPTStream& stream);
    };

    struct SPTLodData
    {
        int32_t mLeafTransitionMethod{};
        float mLeafRadius{};
        float mLeafExponent{};
        float mLeafSize{};
        float mLeafTransition{};
        SPTLodBranchesData mBranches;

        void read(SPTStream& stream);
    };

    struct SPTWindData
    {
        int32_t mFirstWindLevel{};

        void read(SPTStream& stream);
    };

    struct SPTCompositeMapData
    {
        struct Quad
        {
            osg::Vec2f mCorners[4];
        };

        std::vector<Quad> mLeafQuads;
        std::vector<Quad> mBillboardQuads;
        std::vector<Quad> mFrondQuads;
        std::string mUnknown10005;
        uint8_t mUnknown10006{};
        uint8_t mUnknown10007{};

        void read(SPTStream& stream);
    };

    struct SPTFrondTextureMap
    {
        std::string mFilename;
        float mUnknown14003{};
        float mSizeFactor{};
        float mMinOffsetAngle{};
        float mMaxOffsetAngle{};

        void read(SPTStream& stream);
    };

    struct SPTFrondData
    {
        int32_t mGenerationLevel{};
        int32_t mBladesOrExtruded{};
        int32_t mBladeCount{};
        std::string mExtrudedProfile;
        int32_t mExtrudedSegments{};
        int8_t mEnabled{};
        int32_t mNumTextureMaps{};
        int32_t mNumLods{};
        float mMaxArea{};
        float mMinArea{};
        float mFuzziness{};
        float mRetention{};
        int32_t mMinExtrusionSegLength{};
        int32_t mMinExtrusionSegProfile{};
        std::vector<SPTFrondTextureMap> mTextureMaps;

        void read(SPTStream& stream);
    };

    enum class SPTCollisionType
    {
        Sphere,
        Capsule,
        Box
    };

    struct SPTCollisionObject
    {
        SPTCollisionType mType{};
        osg::Vec3f mPosition;
        float mRadius{};
        float mLength{};
        osg::Vec3f mBoxMin;
        osg::Vec3f mBoxMax;
    };

    struct SPTCollisionData
    {
        std::vector<SPTCollisionObject> mObjects;

        void read(SPTStream& stream);
    };

    struct SPTTrunkTilingEntry
    {
        int8_t mRandomVOffset{};
        float mTwist{};
    };

    struct SPTTrunkTilingData
    {
        std::vector<SPTTrunkTilingEntry> mEntries;

        void read(SPTStream& stream);
    };

    struct SPTFlaresEntry
    {
        float mSegmentsPack{};
        int32_t mFlaresNumber{};
        float mFlaresBalance{};
        float mRadialInfluence{};
        float mRadialInfluenceVariance{};
        float mRadialExponent{};
        float mRadialDistance{};
        float mRadialDistanceVariance{};
        float mLengthDistance{};
        float mLengthDistanceVariance{};
        float mLengthExponent{};

        bool readField(SPTStream& stream, int32_t section);
    };

    struct SPTFlaresData
    {
        std::vector<SPTFlaresEntry> mEntries;
        int32_t mFlareSeed{};

        void read(SPTStream& stream);
    };

    struct SPTShadowData
    {
        osg::Vec3f mUnknown18002;
        osg::Vec3f mUnknown18003;
        osg::Vec3f mUnknown18004;
        std::string mSelfShadowMap;

        void read(SPTStream& stream);
    };

    struct SPTUserData
    {
        std::string mText;

        void read(SPTStream& stream);
    };

    struct SPTGlobalCompositeMapData
    {
        std::string mFilename;
        uint8_t mUnknown20003{};
        uint8_t mUnknown20004{};
        std::array<osg::Vec2f, 4> mTexCoords;

        void read(SPTStream& stream);
    };

    // Precomputed leaf cards per leaf LOD, written by the runtime library right after the header
    struct SPTLeafLodCard
    {
        int32_t mUnknown7006{};
        uint8_t mUnknown7007{};
        uint8_t mUnknown7008{};
        osg::Vec3f mUnknown7010;
        uint8_t mUnknown7011{};
        uint8_t mUnknown7012{};
        int32_t mUnknown7013{};
        osg::Vec3f mUnknown7015;
        float mUnknown7016{};
    };

    struct SPTLeafLodData
    {
        std::vector<std::vector<SPTLeafLodCard>> mLods;

        void read(SPTStream& stream);
    };

    // SpeedTree 4.1+ blocks. Version 4.1/4.2 files carry them after 22000;
    // Oblivion's older runtime stops before them. Names follow the 4.2 editor UI.

    struct SPTLightSeamEntry
    {
        float mUnknown23002{};
        float mUnknown23003{};
    };

    struct SPTLightSeamData
    {
        std::vector<SPTLightSeamEntry> mEntries;

        void read(SPTStream& stream);
    };

    struct SPTLeafPlacementData
    {
        float mUnknown24002{};
        int32_t mUnknown24003{};

        void read(SPTStream& stream);
    };

    struct SPTSupplementalFrondData
    {
        float mGenerationDistance{};
        int32_t mGenerationDepth{};
        int32_t mAboveCondition{};
        int32_t mBelowCondition{};
        int32_t mLengthSegmentOverride{};
        int8_t mLengthSegmentOverrideEnabled{};

        void read(SPTStream& stream);
    };

    struct SPTSupplementalBranchLevel
    {
        float mRoughnessAmount{};
        float mSegmentsLengthKeep{};
        float mSegmentsCrossSectionKeep{};
        float mGenerationDistance{};
        int32_t mGenerationDepth{};
        int8_t mForkEnabled{};
        float mForkBias{};
        float mForkAngle{};
        int32_t mForkLimit{};
        Curve mUnknown26013;
        Curve mUnknown26014;
        float mRoughnessVertFrequency{};
        float mRoughnessHorizFrequency{};
        float mRoughnessAmountVariance{};
        Curve mUnknown26018;
        Curve mUnknown26019;
        Curve mUnknown26020;
        float mRoughnessGnarl{};
        Curve mUnknown26022;
        int8_t mRoughnessGnarlUnison{};

        bool readField(SPTStream& stream, int32_t section);
    };

    struct SPTSupplementalBranchData
    {
        std::vector<SPTSupplementalBranchLevel> mLevels;

        void read(SPTStream& stream);
    };

    struct SPTFloorData
    {
        int8_t mEnabled{};
        float mValue{};
        int32_t mLevel{};
        float mExponent{};
        float mBias{};

        void read(SPTStream& stream);
    };

    struct SPTLeafSmoothingData
    {
        int8_t mEnabled{};
        float mAmount{};
        int32_t mDimmingDepth{};

        void read(SPTStream& stream);
    };

    struct SPTStandardShaderData
    {
        float mBranchLightScalar{};
        float mFrondLightScalar{};
        float mLeafLightScalar{};
        float mGlobalLightScalar{};
        float mAmbientLightScalar{};
        float mBillboardDarkSide{};
        float mBillboardAmbient{};
        float mBillboardBrightSide{};

        void read(SPTStream& stream);
    };

    struct SPTRootData
    {
        int32_t mRootLevel{};
        float mGenerationFirst{};
        float mGenerationLast{};
        float mGenerationFrequency{};
        SPTLevelData mLevel;
        SPTTrunkTilingEntry mTrunkTiling;
        SPTFlaresEntry mFlares;
        SPTLightSeamEntry mLightSeam;
        SPTSupplementalBranchLevel mSupplemental;

        void read(SPTStream& stream);
    };

    struct SPTTexCoordControls
    {
        float mSTiling{};
        float mTTiling{};
        int8_t mSTilingAbsolute{};
        int8_t mTTilingAbsolute{};
        int8_t mRandomTOffset{};
        float mTwist{};
        float mSOffset{};
        float mTOffset{};
        int8_t mClampS{};
        int8_t mClampT{};
        float mClampSLeft{};
        float mClampSRight{};
        float mClampTBottom{};
        float mClampTTop{};
        int8_t mSynchToDiffuse{};
    };

    struct SPTTexCoordData
    {
        std::vector<SPTTexCoordControls> mLayers;

        void read(SPTStream& stream);
    };

    struct SPTTextureMap
    {
        // Diffuse, detail, normal, height, specular, user 1, user 2
        std::array<std::string, 7> mLayers;
    };

    struct SPTMapBank
    {
        SPTTextureMap mBranchMap;
        std::vector<SPTTextureMap> mLeafMaps;
        std::vector<SPTTextureMap> mFrondMaps;
        // The composite and the separate billboard map, not yet known which is which
        SPTTextureMap mUnknown60005;
        SPTTextureMap mUnknown60009;
        std::string mSelfShadowMap;

        void read(SPTStream& stream);
    };

    struct SPTLeafMeshVertex
    {
        osg::Vec3f mPosition;
        osg::Vec3f mNormal;
        osg::Vec3f mBinormal;
        osg::Vec3f mTangent;
        osg::Vec2f mTexCoord;
    };

    struct SPTLeafMesh
    {
        std::string mName;
        std::vector<SPTLeafMeshVertex> mVertices;
        std::vector<int32_t> mIndices;
    };

    struct SPTMeshInfo
    {
        std::vector<SPTLeafMesh> mMeshes;

        void read(SPTStream& stream);
    };

    struct SPTLeafMeshEntry
    {
        int8_t mUnknown72001{};
        int32_t mUnknown72002{};
        int32_t mUnknown72003{};
        float mUnknown72005{};
        float mUnknown72006{};
    };

    struct SPTLeafMeshData
    {
        std::vector<SPTLeafMeshEntry> mEntries;

        void read(SPTStream& stream);
    };

    struct SPTCollisionExtraData
    {
        std::vector<osg::Vec3f> mObjects;

        void read(SPTStream& stream);
    };

    struct SPTLodFadeData
    {
        float mBranchFadeDistance{};
        float mFrondFadeDistance{};
        int8_t mFadeExtrudedFronds{};
        float mBillboardTransition{};

        void read(SPTStream& stream);
    };

    struct SPTHeader
    {
        std::string mVersionId;
        std::string mTextureMap;
        float mLodFarDistance{};
        float mLodNearDistance{};
        float mSize{};
        float mSizeVariance{};
        int32_t mSeed{};
        int32_t mNumLevels{};
        int8_t mUnknown2002{};
        int32_t mUnknown2004{};

        std::vector<SPTLevelData> mLevelData;
        SPTLeafData mLeafData;

        void read(SPTStream& stream);
    };

    struct SPTFile
    {
        SPTHeader mHeader;
        SPTLeafLodData mLeafLods;
        SPTLightingData mLighting;
        SPTLodData mLod;
        SPTWindData mWind;
        SPTCompositeMapData mCompositeMap;
        SPTFrondData mFrond;
        SPTCollisionData mCollision;
        SPTTrunkTilingData mTrunkTiling;
        SPTFlaresData mFlares;
        SPTShadowData mShadow;
        SPTUserData mUserData;
        SPTGlobalCompositeMapData mCompositeMapGlobal;
        float mLeafRockAngleScalar{};
        float mLeafRustleAngleScalar{};
        uint8_t mBranchWeightingFlexibility{};

        SPTLightSeamData mLightSeam;
        SPTLeafPlacementData mLeafPlacement;
        SPTSupplementalFrondData mSupplementalFrond;
        SPTSupplementalBranchData mSupplementalBranch;
        SPTFloorData mFloor;
        SPTLeafSmoothingData mLeafSmoothing;
        int32_t mClusterFirstVisibleLevel{};
        SPTStandardShaderData mStandardShader;
        SPTRootData mRoots;
        SPTTexCoordData mTexCoords;
        SPTMapBank mMapBank;
        SPTMeshInfo mMeshInfo;
        SPTLeafMeshData mLeafMeshes;
        SPTCollisionExtraData mCollisionExtra;
        float mOrientationAngle{};
        SPTLodFadeData mLodFade;

        SPTFile() = default;
    };

    using SPTFilePtr = std::shared_ptr<const Spt::SPTFile>;

    SPTFilePtr parse(Files::IStreamPtr&& stream, std::string_view name);
}

#endif
