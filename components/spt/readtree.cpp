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
                    throw std::runtime_error(std::format("Unknown section {} in leaf texture map block", section));
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
                                throw std::runtime_error(std::format("Unknown section {} in leaf 1009 block", inner));
                        }
                    }
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in leaf properties block", section));
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
                    throw std::runtime_error(std::format("Unknown section {} in frond texture map block", section));
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
                    throw std::runtime_error(std::format("Unknown section {} in frond block", section));
            }
        }
    }
}