#include "file.hpp"

#include <format>
#include <stdexcept>

#include "stream.hpp"

namespace Spt
{
    void SPTLightingMaterial::read(SPTStream& stream)
    {
        stream.readVec3f(mDiffuse);
        stream.readVec3f(mAmbient);
        stream.readVec3f(mSpecular);
        stream.readVec3f(mEmissive);
        mShine = stream.readFloat();
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
                    mBranchStyle = stream.readInt();
                    break;
                case 8003:
                    mBranchMaterial.read(stream);
                    break;
                case 8004:
                    mLeafStyle = stream.readInt();
                    break;
                case 8005:
                    mLeafMaterial.read(stream);
                    break;
                case 8006:
                    mLeafLightingAdjust = stream.readFloat();
                    break;
                case 8007:
                    mLeafStaticShadows = stream.readInt();
                    break;
                case 8008:
                    mFrondStyle = stream.readInt();
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
                    mBranchNumLods = stream.readInt();
                    break;
                case 9008:
                    mBranchMinVol = stream.readFloat();
                    break;
                case 9010:
                    mLeafReduction = stream.readFloat();
                    break;
                case 9011:
                    mLeafNumLods = stream.readInt();
                    break;
                case 9012:
                    mBranchMaxVol = stream.readFloat();
                    break;
                case 9013:
                    mBranchFuzziness = stream.readFloat();
                    break;
                case 9014:
                    mBranchRetention = stream.readFloat();
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
                    mLeafTransitionMethod = stream.readInt();
                    break;
                case 9003:
                    mLeafRadius = stream.readFloat();
                    break;
                case 9004:
                    mLeafExponent = stream.readFloat();
                    break;
                case 9009:
                    mLeafSize = stream.readFloat();
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
                    mFirstWindLevel = stream.readInt();
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
            entry.mRandomVOffset = stream.readByte();
            section = stream.readSection();
            if (section != 15003)
                throw std::runtime_error(std::format("Expected section 15003, got {}", section));
            entry.mTwist = stream.readFloat();
            mEntries.push_back(std::move(entry));
        }
    }
}