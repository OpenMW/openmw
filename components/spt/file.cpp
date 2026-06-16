#include "file.hpp"

#include <format>
#include <stdexcept>

#include "stream.hpp"

namespace Spt
{
    void readBezierSection(SPTLevelData& level, SPTStream& stream, int32_t section)
    {
        switch (section)
        {
            case 6000:
                level.mDisturbance = stream.readIntString();
                return;
            case 6001:
                level.mGravity = stream.readIntString();
                return;
            case 6002:
                level.mFlexibility = stream.readIntString();
                return;
            case 6003:
                level.mFlexibilityProfile = stream.readIntString();
                return;
            case 6004:
                level.mLength = stream.readIntString();
                return;
            case 6005:
                level.mRadius = stream.readIntString();
                return;
            case 6006:
                level.mRadiusProfile = stream.readIntString();
                return;
            case 6007:
                level.mStartAngle = stream.readIntString();
                return;
            case 6017:
                level.mGravityProfile = stream.readIntString();
                return;
            case 6008:
                level.mSegmentsCrossSection = stream.readInt();
                return;
            case 6009:
                level.mSegmentsLength = stream.readInt();
                return;
            case 6010:
                level.mGenerationFirst = stream.readFloat();
                return;
            case 6011:
                level.mGenerationLast = stream.readFloat();
                return;
            case 6012:
                level.mGenerationFrequency = stream.readFloat();
                return;
            case 6013:
                level.mTextureTilingU = stream.readFloat();
                return;
            case 6014:
                level.mTextureTilingV = stream.readFloat();
                return;
            case 6015:
                level.mTextureTilingUAbs = stream.readByte();
                return;
            case 6016:
                level.mTextureTilingVAbs = stream.readByte();
                return;
            default:
                throw std::runtime_error(std::format("Unknown bezier section {} in SPT file", section));
        }
    }

    void SPTLevelData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 1017)
                return;
            readBezierSection(*this, stream, section);
        }
    }

    void readTreeProperties(SPTHeader& header, SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            switch (section)
            {
                case 2000:
                    header.mTextureMap = stream.readIntString();
                    break;
                case 2001:
                    header.mLodFarDistance = stream.readFloat();
                    break;
                case 2002:
                    header.mUnknown2002 = stream.readByte();
                    break;
                case 2003:
                    header.mLodNearDistance = stream.readFloat();
                    break;
                case 2005:
                    header.mSeed = stream.readInt();
                    break;
                case 2006:
                    header.mSize = stream.readFloat();
                    break;
                case 2007:
                    header.mSizeVariance = stream.readFloat();
                    break;
                case 1014:
                    header.mNumLevels = stream.readInt();
                    break;
                case 1015:
                    // end of 1014 block, continue til 1003
                    break;
                case 1016:
                {
                    SPTLevelData level;
                    level.read(stream);
                    header.mLevelData.push_back(std::move(level));
                    break;
                }
                case 1003:
                    return;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in tree properties block", section));
            }
        }
    }

    void discardSections(SPTStream& stream, int32_t endSection)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == endSection)
                return;
            switch (section)
            {
                case 5000: stream.skip(12); break;
                case 5001: stream.skip(12); break;
                case 5002: stream.skip(12); break;
                case 5003: stream.skip(12); break;
                case 5004: stream.skip(12); break;
                case 5005: stream.skip(4); break;
                case 5006: stream.skip(1); break;
                default:
                    throw std::runtime_error(std::format(
                        "Unknown section {} in discarded block ending at {}", section, endSection));
            }
        }
    }

    void readLeafTextureMap(SPTLeafTextureMap& tex, SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 1008)
                return;
            switch (section)
            {
                case 4000:
                    tex.mMakeBlossom = stream.readByte();
                    break;
                case 4001:
                    stream.readVec3f(tex.mColor);
                    break;
                case 4002:
                    tex.mOrientationVariance = stream.readFloat();
                    break;
                case 4003:
                    tex.mFilename = stream.readIntString();
                    break;
                case 4004:
                    stream.readVec3f(tex.mOrigin);
                    break;
                case 4005:
                    stream.readVec3f(tex.mSize);
                    break;
                case 4006:
                    stream.readVec3f(tex.mUnknown4006);
                    break;
                case 4007:
                    tex.mUnknown4007 = stream.readFloat();
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in leaf texture map block", section));
            }
        }
    }

    void SPTLeafData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 1005)
                return;
            switch (section)
            {
                case 3000:
                    mBlossomDistance = stream.readFloat();
                    break;
                case 3001:
                    mBlossomDepth = stream.readInt();
                    break;
                case 3002:
                    mBlossomWeight = stream.readFloat();
                    break;
                case 3003:
                    mUnknown3003 = stream.readByte();
                    break;
                case 3004:
                    mUnknown3004 = stream.readFloat();
                    break;
                case 3009:
                    mLightingDimmingToggle = stream.readByte();
                    break;
                case 3010:
                    mLightingDimming = stream.readFloat();
                    break;
                case 3005:
                    mUnknown3005 = stream.readFloat();
                    break;
                case 3006:
                    mUnknown3006 = stream.readByte();
                    break;
                case 3007:
                    mPlacementTolerance = stream.readFloat();
                    break;
                case 3008:
                    mPlacementCollision = stream.readInt();
                    break;
                case 1009:
                    while (true)
                    {
                        int32_t inner = stream.readSection();
                        if (inner == 1010)
                            break;
                        switch (inner)
                        {
                            case 1006:
                                mNumLeafTextures = stream.readInt();
                                break;
                            case 1007:
                            {
                                SPTLeafTextureMap tex;
                                readLeafTextureMap(tex, stream);
                                mLeafTextures.push_back(std::move(tex));
                                break;
                            }
                            default:
                                throw std::runtime_error(
                                    std::format("Unknown section {} in leaf 1009 block", inner));
                        }
                    }
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in leaf properties block", section));
            }
        }
    }

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
                    throw std::runtime_error(
                        std::format("Unknown section {} in lighting block", section));
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
                    throw std::runtime_error(
                        std::format("Unknown section {} in LOD branches block", section));
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
                    throw std::runtime_error(
                        std::format("Unknown section {} in LOD block", section));
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
                    throw std::runtime_error(
                        std::format("Unknown section {} in wind block", section));
            }
        }
    }

    namespace
    {
        std::vector<SPTCompositeMapData::Quad> readQuads(SPTStream& stream)
        {
            int32_t n = stream.readInt();
            std::vector<SPTCompositeMapData::Quad> quads(n);
            for (auto& quad : quads)
                for (auto& corner : quad.mCorners)
                    stream.readVec2f(corner);
            return quads;
        }
    }

    void SPTCompositeMapData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 10001)
                return;
            switch (section)
            {
                case 10002:
                    mLeafQuads = readQuads(stream);
                    break;
                case 10003:
                    mBillboardQuads = readQuads(stream);
                    break;
                case 10004:
                    mFrondQuads = readQuads(stream);
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in composite map block", section));
            }
        }
    }

    void SPTFrondTextureMap::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 14001)
                return;
            switch (section)
            {
                case 14002:
                    mFilename = stream.readIntString();
                    break;
                case 14003:
                    mUnknown14003 = stream.readFloat();
                    break;
                case 14004:
                    mSizeFactor = stream.readFloat();
                    break;
                case 14005:
                    mMinOffsetAngle = stream.readFloat();
                    break;
                case 14006:
                    mMaxOffsetAngle = stream.readFloat();
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in frond texture map block", section));
            }
        }
    }

    void SPTFrondData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 13001)
                return;
            switch (section)
            {
                case 13002:
                    mGenerationLevel = stream.readInt();
                    break;
                case 13003:
                    mBladesOrExtruded = stream.readInt();
                    break;
                case 13004:
                    mBladeCount = stream.readInt();
                    break;
                case 13005:
                    mExtrudedProfile = stream.readIntString();
                    break;
                case 13006:
                    mExtrudedSegments = stream.readInt();
                    break;
                case 13007:
                    mEnabled = stream.readByte();
                    break;
                case 13008:
                    mNumTextureMaps = stream.readInt();
                    break;
                case 13009:
                    mNumLods = stream.readInt();
                    break;
                case 13010:
                    mMaxArea = stream.readFloat();
                    break;
                case 13011:
                    mMinArea = stream.readFloat();
                    break;
                case 13012:
                    mFuzziness = stream.readFloat();
                    break;
                case 13013:
                    mRetention = stream.readFloat();
                    break;
                case 14007:
                    mMinExtrusionSegLength = stream.readInt();
                    break;
                case 14008:
                    mMinExtrusionSegProfile = stream.readInt();
                    break;
                case 14000:
                {
                    SPTFrondTextureMap tex;
                    tex.read(stream);
                    mTextureMaps.push_back(std::move(tex));
                    break;
                }
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in frond block", section));
            }
        }
    }

    void SPTCollisionData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 12001)
                return;
            switch (section)
            {
                case 12002:
                {
                    SPTCollisionObject obj;
                    obj.mType = SPTCollisionType::Sphere;
                    stream.readVec3f(obj.mPosition);
                    obj.mRadius = stream.readFloat();
                    mObjects.push_back(std::move(obj));
                    break;
                }
                case 12003:
                {
                    SPTCollisionObject obj;
                    obj.mType = SPTCollisionType::Capsule;
                    stream.readVec3f(obj.mPosition);
                    obj.mRadius = stream.readFloat();
                    obj.mLength = stream.readFloat();
                    mObjects.push_back(std::move(obj));
                    break;
                }
                case 12004:
                {
                    SPTCollisionObject obj;
                    obj.mType = SPTCollisionType::Box;
                    stream.readVec3f(obj.mBoxMin);
                    stream.readVec3f(obj.mBoxMax);
                    mObjects.push_back(std::move(obj));
                    break;
                }
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in collision block", section));
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
                throw std::runtime_error(
                    std::format("Expected section 15002, got {}", section));
            SPTTrunkTilingEntry entry;
            entry.mRandomVOffset = stream.readByte();
            section = stream.readSection();
            if (section != 15003)
                throw std::runtime_error(
                    std::format("Expected section 15003, got {}", section));
            entry.mTwist = stream.readFloat();
            mEntries.push_back(std::move(entry));
        }
    }

    void SPTFlaresData::read(SPTStream& stream)
    {
        SPTFlaresEntry entry;

        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 16001)
                return;

            switch (section)
            {
                case 16002: entry.mSegmentsPack = stream.readFloat(); break;
                case 16003: entry.mFlaresNumber = stream.readInt(); break;
                case 16004: entry.mFlaresBalance = stream.readFloat(); break;
                case 16005: entry.mRadialInfluence = stream.readFloat(); break;
                case 16006: entry.mRadialInfluenceVariance = stream.readFloat(); break;
                case 16007: entry.mRadialExponent = stream.readFloat(); break;
                case 16008: entry.mRadialDistance = stream.readFloat(); break;
                case 16009: entry.mRadialDistanceVariance = stream.readFloat(); break;
                case 16010: entry.mLengthDistance = stream.readFloat(); break;
                case 16011: entry.mLengthDistanceVariance = stream.readFloat(); break;
                case 16012:
                    entry.mLengthExponent = stream.readFloat();
                    mEntries.push_back(std::move(entry));
                    entry = {};
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in flares block", section));
            }
        }
    }

    void SPTShadowData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 18001)
                return;
            switch (section)
            {
                case 18002:
                    stream.readVec3f(mUnknown18002);
                    break;
                case 18003:
                    stream.readVec3f(mUnknown18003);
                    break;
                case 18004:
                    stream.readVec3f(mUnknown18004);
                    break;
                case 18005:
                    mSelfShadowMap = stream.readIntString();
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in shadow block", section));
            }
        }
    }

    void SPTHeader::read(SPTStream& stream)
    {
        mVersionId = stream.readIntString();
        if (mVersionId != "__IdvSpt_02_")
            throw std::runtime_error(std::format("Unsupported SPT version '{}'", mVersionId));

        while (true)
        {
            int32_t section = stream.readSection();
            switch (section)
            {
                case 1002:
                    readTreeProperties(*this, stream);
                    break;
                case 1004:
                    mLeafData.read(stream);
                    break;
                case 1011:
                    discardSections(stream, 1012);
                    break;
                case 1001:
                    return;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in header block", section));
            }
        }
    }

    SPTFilePtr parse(Files::IStreamPtr&& inputStream)
    {
        auto file = std::make_shared<SPTFile>();
        SPTStream stream(std::move(inputStream));

        while (true)
        {
            int32_t section = stream.readSection();
            switch (section)
            {
                case 1000:
                    file->mHeader.read(stream);
                    break;
                case 8000:
                    file->mLighting.read(stream);
                    break;
                case 9000:
                    file->mLod.read(stream);
                    break;
                case 11000:
                    file->mWind.read(stream);
                    break;
                case 10000:
                    file->mCompositeMap.read(stream);
                    break;
                case 12000:
                    file->mCollision.read(stream);
                    break;
                case 13000:
                    file->mFrond.read(stream);
                    break;
                case 15000:
                    file->mTrunkTiling.read(stream);
                    break;
                case 16000:
                    file->mFlares.read(stream);
                    break;
                case 16013:
                    file->mFlares.mFlareSeed = stream.readInt();
                    break;
                case 16014:
                    file->mLod.mLeafTransition = stream.readFloat();
                    break;
                case 18000:
                    file->mShadow.read(stream);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown top-level section {} in SPT file", section));
            }
        }
    }
}
