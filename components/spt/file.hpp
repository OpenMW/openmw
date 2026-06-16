#ifndef OPENMW_COMPONENTS_SPT_FILE_HPP
#define OPENMW_COMPONENTS_SPT_FILE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/files/istreamptr.hpp>

namespace Spt
{
    class SPTStream;

    struct SPTLevelData
    {
        std::string mDisturbance;
        std::string mGravity;
        std::string mFlexibility;
        std::string mFlexibilityProfile;
        std::string mLength;
        std::string mRadius;
        std::string mRadiusProfile;
        std::string mStartAngle;
        std::string mGravityProfile;

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
        struct Quad { osg::Vec2f mCorners[4]; };

        std::vector<Quad> mLeafQuads;
        std::vector<Quad> mBillboardQuads;
        std::vector<Quad> mFrondQuads;

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

        SPTFile() = default;
    };

    using SPTFilePtr = std::shared_ptr<const Spt::SPTFile>;

    SPTFilePtr parse(Files::IStreamPtr&& stream);
}

#endif
