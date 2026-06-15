#ifndef OPENMW_COMPONENTS_SPT_FILE_HPP
#define OPENMW_COMPONENTS_SPT_FILE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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

    struct SPTFile
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

        SPTFile() = default;
    };

    using SPTFilePtr = std::shared_ptr<const Spt::SPTFile>;

    SPTFilePtr parse(Files::IStreamPtr&& stream);
}

#endif
