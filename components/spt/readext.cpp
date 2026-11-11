#include "file.hpp"

#include <format>
#include <stdexcept>

#include "stream.hpp"

namespace Spt
{
    namespace
    {
        SPTTextureMap readTextureMap(SPTStream& stream)
        {
            stream.expectSection(70000);
            SPTTextureMap map;
            while (true)
            {
                const int32_t section = stream.readSection();
                if (section == 70001)
                    return map;
                if (section < 70002 || section > 70008)
                    throw std::runtime_error(std::format("Unknown section {} in texture map", section));
                map.mLayers[section - 70002] = stream.readIntString();
            }
        }

        std::vector<SPTTextureMap> readTextureMaps(SPTStream& stream, int32_t countSection)
        {
            stream.expectSection(countSection);
            uint32_t count;
            stream.read(count);
            std::vector<SPTTextureMap> maps;
            for (uint32_t i = 0; i < count; ++i)
                maps.push_back(readTextureMap(stream));
            return maps;
        }
    }

    void SPTLightSeamData::read(SPTStream& stream)
    {
        while (true)
        {
            const int32_t section = stream.readSection();
            if (section == 23001)
                return;
            if (section != 23002)
                throw std::runtime_error(std::format("Expected section 23002, got {}", section));
            SPTLightSeamEntry entry;
            stream.read(entry.mUnknown23002);
            stream.expectSection(23003);
            stream.read(entry.mUnknown23003);
            mEntries.push_back(entry);
        }
    }

    void SPTLeafPlacementData::read(SPTStream& stream)
    {
        stream.expectSection(24002);
        stream.read(mUnknown24002);
        stream.expectSection(24003);
        stream.read(mUnknown24003);
        stream.expectSection(24001);
    }

    void SPTSupplementalFrondData::read(SPTStream& stream)
    {
        while (true)
        {
            const int32_t section = stream.readSection();
            if (section == 25001)
                return;
            switch (section)
            {
                case 25002:
                    stream.read(mGenerationDistance);
                    break;
                case 25003:
                    stream.read(mUnknown25003);
                    break;
                case 25004:
                    stream.read(mAboveCondition);
                    break;
                case 25005:
                    stream.read(mBelowCondition);
                    break;
                case 25006:
                    stream.read(mLengthSegmentOverride);
                    break;
                case 25007:
                    stream.read(mLengthSegmentOverrideEnabled);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in supplemental frond block", section));
            }
        }
    }

    void SPTFloorData::read(SPTStream& stream)
    {
        while (true)
        {
            const int32_t section = stream.readSection();
            if (section == 27001)
                return;
            switch (section)
            {
                case 27002:
                    stream.read(mEnabled);
                    break;
                case 27003:
                    stream.read(mValue);
                    break;
                case 27004:
                    stream.read(mLevel);
                    break;
                case 27005:
                    stream.read(mExponent);
                    break;
                case 27006:
                    stream.read(mBias);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in floor block", section));
            }
        }
    }

    void SPTLeafSmoothingData::read(SPTStream& stream)
    {
        while (true)
        {
            const int32_t section = stream.readSection();
            if (section == 28001)
                return;
            switch (section)
            {
                case 28002:
                    stream.read(mEnabled);
                    break;
                case 28003:
                    stream.read(mAmount);
                    break;
                case 28004:
                    stream.read(mDimmingDepth);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in leaf smoothing block", section));
            }
        }
    }

    void SPTStandardShaderData::read(SPTStream& stream)
    {
        while (true)
        {
            const int32_t section = stream.readSection();
            if (section == 30001)
                return;
            switch (section)
            {
                case 30002:
                    stream.read(mBranchLightScalar);
                    break;
                case 30003:
                    stream.read(mFrondLightScalar);
                    break;
                case 30004:
                    stream.read(mLeafLightScalar);
                    break;
                case 30005:
                    stream.read(mGlobalLightScalar);
                    break;
                case 30006:
                    stream.read(mAmbientLightScalar);
                    break;
                case 30007:
                    stream.read(mBillboardDarkSide);
                    break;
                case 30008:
                    stream.read(mBillboardAmbient);
                    break;
                case 30009:
                    stream.read(mBillboardBrightSide);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in standard shader block", section));
            }
        }
    }

    void SPTTexCoordData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 50001)
                return;
            if (section != 50002)
                throw std::runtime_error(std::format("Expected section 50002, got {}", section));
            SPTTexCoordControls& layer = mLayers.emplace_back();
            while ((section = stream.readSection()) != 50003)
            {
                switch (section)
                {
                    case 50004:
                        stream.read(layer.mSTiling);
                        break;
                    case 50005:
                        stream.read(layer.mTTiling);
                        break;
                    case 50006:
                        stream.read(layer.mSTilingAbsolute);
                        break;
                    case 50007:
                        stream.read(layer.mTTilingAbsolute);
                        break;
                    case 50008:
                        stream.read(layer.mTwist);
                        break;
                    case 50009:
                        stream.read(layer.mRandomTOffset);
                        break;
                    case 50010:
                        stream.read(layer.mTOffset);
                        break;
                    case 50011:
                        stream.read(layer.mClampS);
                        break;
                    case 50012:
                        stream.read(layer.mClampT);
                        break;
                    case 50013:
                        stream.read(layer.mClampSLeft);
                        break;
                    case 50014:
                        stream.read(layer.mClampSRight);
                        break;
                    case 50015:
                        stream.read(layer.mClampTBottom);
                        break;
                    case 50016:
                        stream.read(layer.mClampTTop);
                        break;
                    case 50017:
                        stream.read(layer.mSOffset);
                        break;
                    case 50018:
                        stream.read(layer.mSynchToDiffuse);
                        break;
                    default:
                        throw std::runtime_error(std::format("Unknown section {} in tex coord block", section));
                }
            }
        }
    }

    void SPTMapBank::read(SPTStream& stream)
    {
        while (true)
        {
            const int32_t section = stream.readSection();
            switch (section)
            {
                case 60001:
                    return;
                case 60002:
                    mBranchMap = readTextureMap(stream);
                    break;
                case 60003:
                    mLeafMaps = readTextureMaps(stream, 60007);
                    break;
                case 60004:
                    mFrondMaps = readTextureMaps(stream, 60008);
                    break;
                case 60005:
                    mUnknown60005 = readTextureMap(stream);
                    break;
                case 60006:
                    mSelfShadowMap = stream.readIntString();
                    break;
                case 60009:
                    mUnknown60009 = readTextureMap(stream);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in map bank", section));
            }
        }
    }

    void SPTMeshInfo::read(SPTStream& stream)
    {
        stream.expectSection(71001);
        uint32_t numMeshes;
        stream.read(numMeshes);
        for (uint32_t i = 0; i < numMeshes; ++i)
        {
            SPTLeafMesh& mesh = mMeshes.emplace_back();
            stream.expectSection(71002);
            stream.expectSection(71003);
            mesh.mName = stream.readIntString();
            stream.expectSection(71004);
            uint32_t numVertices;
            stream.read(numVertices);
            for (uint32_t j = 0; j < numVertices; ++j)
            {
                SPTLeafMeshVertex& vertex = mesh.mVertices.emplace_back();
                stream.expectSection(71005);
                stream.expectSection(71006);
                stream.read(vertex.mPosition);
                stream.expectSection(71007);
                stream.read(vertex.mNormal);
                stream.expectSection(71008);
                stream.read(vertex.mBinormal);
                stream.expectSection(71009);
                stream.read(vertex.mTangent);
                stream.expectSection(71010);
                stream.read(vertex.mTexCoord);
                stream.expectSection(71011);
            }
            stream.expectSection(71012);
            uint32_t numIndices;
            stream.read(numIndices);
            for (uint32_t j = 0; j < numIndices; ++j)
            {
                stream.expectSection(71013);
                stream.read(mesh.mIndices.emplace_back());
            }
            stream.expectSection(71014);
        }
        stream.expectSection(71015);
    }

    void SPTLeafMeshData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 72004)
                return;
            if (section != 7000)
                throw std::runtime_error(std::format("Expected section 7000, got {}", section));
            SPTLeafMeshEntry& entry = mEntries.emplace_back();
            while ((section = stream.readSection()) != 7001)
            {
                switch (section)
                {
                    case 72001:
                        stream.read(entry.mUnknown72001);
                        break;
                    case 72002:
                        stream.read(entry.mUnknown72002);
                        break;
                    case 72003:
                        stream.read(entry.mUnknown72003);
                        break;
                    case 72005:
                        stream.read(entry.mUnknown72005);
                        break;
                    case 72006:
                        stream.read(entry.mUnknown72006);
                        break;
                    default:
                        throw std::runtime_error(std::format("Unknown section {} in leaf mesh block", section));
                }
            }
        }
    }

    void SPTCollisionExtraData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 73001)
                return;
            if (section != 73002)
                throw std::runtime_error(std::format("Expected section 73002, got {}", section));
            osg::Vec3f& object = mObjects.emplace_back();
            while ((section = stream.readSection()) != 73003)
            {
                if (section < 73004 || section > 73006)
                    throw std::runtime_error(std::format("Unknown section {} in collision extra block", section));
                stream.read(object[section - 73004]);
            }
        }
    }

    void SPTLodFadeData::read(SPTStream& stream)
    {
        while (true)
        {
            const int32_t section = stream.readSection();
            if (section == 75001)
                return;
            switch (section)
            {
                case 75002:
                    stream.read(mBranchFadeDistance);
                    break;
                case 75003:
                    stream.read(mFrondFadeDistance);
                    break;
                case 75004:
                    stream.read(mFadeExtrudedFronds);
                    break;
                case 75005:
                    stream.read(mBillboardTransition);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in LOD fade block", section));
            }
        }
    }
}
