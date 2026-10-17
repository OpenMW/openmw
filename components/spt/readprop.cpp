#include "file.hpp"

#include <format>
#include <stdexcept>

#include "stream.hpp"

namespace Spt
{
    void SPTLightingMaterial::read(SPTStream& stream)
    {
        stream.read(mDiffuse);
        stream.read(mAmbient);
        stream.read(mSpecular);
        stream.read(mEmissive);
        stream.read(mShine);
    }

    void SPTLightingData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 8001)
                return;
            switch (section)
            {
                case 8002:
                    stream.read(mBranchStyle);
                    break;
                case 8003:
                    mBranchMaterial.read(stream);
                    break;
                case 8004:
                    stream.read(mLeafStyle);
                    break;
                case 8005:
                    mLeafMaterial.read(stream);
                    break;
                case 8006:
                    stream.read(mLeafLightingAdjust);
                    break;
                case 8007:
                    stream.read(mLeafStaticShadows);
                    break;
                case 8008:
                    stream.read(mFrondStyle);
                    break;
                case 8009:
                    mFrondMaterial.read(stream);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in lighting block", section));
            }
        }
    }

    void SPTLodBranchesData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 9006)
                return;
            switch (section)
            {
                case 9007:
                    stream.read(mBranchNumLods);
                    break;
                case 9008:
                    stream.read(mBranchMinVol);
                    break;
                case 9010:
                    stream.read(mLeafReduction);
                    break;
                case 9011:
                    stream.read(mLeafNumLods);
                    break;
                case 9012:
                    stream.read(mBranchMaxVol);
                    break;
                case 9013:
                    stream.read(mBranchFuzziness);
                    break;
                case 9014:
                    stream.read(mBranchRetention);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in LOD branches block", section));
            }
        }
    }

    void SPTLodData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 9001)
                return;
            switch (section)
            {
                case 9002:
                    stream.read(mLeafTransitionMethod);
                    break;
                case 9003:
                    stream.read(mLeafRadius);
                    break;
                case 9004:
                    stream.read(mLeafExponent);
                    break;
                case 9009:
                    stream.read(mLeafSize);
                    break;
                case 9005:
                    mBranches.read(stream);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in LOD block", section));
            }
        }
    }

    void SPTWindData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 11001)
                return;
            switch (section)
            {
                case 11002:
                    stream.read(mFirstWindLevel);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in wind block", section));
            }
        }
    }

    void SPTTrunkTilingData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 15001)
                return;
            if (section != 15002)
                throw std::runtime_error(std::format("Expected section 15002, got {}", section));
            SPTTrunkTilingEntry entry;
            stream.read(entry.mRandomVOffset);
            section = stream.readSection();
            if (section != 15003)
                throw std::runtime_error(std::format("Expected section 15003, got {}", section));
            stream.read(entry.mTwist);
            mEntries.push_back(std::move(entry));
        }
    }
}