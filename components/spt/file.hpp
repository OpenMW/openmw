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

    struct SPTShadowRelatedData
    {
        std::string mUnknown19002;

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

        std::vector<SPTLevelData> mLevelData;
        SPTLeafData mLeafData;

        void read(SPTStream& stream);
    };

    struct SPTFile
    {
        SPTHeader mHeader;
        SPTLightingData mLighting;
        SPTLodData mLod;
        SPTWindData mWind;
        SPTCompositeMapData mCompositeMap;
        SPTFrondData mFrond;
        SPTCollisionData mCollision;
        SPTTrunkTilingData mTrunkTiling;
        SPTFlaresData mFlares;
        SPTShadowData mShadow;
        SPTShadowRelatedData mShadowRelated;
        SPTGlobalCompositeMapData mCompositeMapGlobal;
        float mLeafRockAngleScalar{};
        float mLeafRustleAngleScalar{};
        uint8_t mBranchWeightingFlexibility{};

        SPTFile() = default;
    };

    using SPTFilePtr = std::shared_ptr<const Spt::SPTFile>;

    SPTFilePtr parse(Files::IStreamPtr&& stream, std::string_view name);
}

#endif
