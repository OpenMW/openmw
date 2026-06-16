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
                default:
                    throw std::runtime_error(std::format("Unknown top-level section {} in SPT file", section));
            }
        }
    }
}
