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

    SPTFilePtr parse(Files::IStreamPtr&& inputStream)
    {
        auto file = std::make_shared<SPTFile>();
        SPTStream stream(std::move(inputStream));

        {
            int32_t section = stream.readSection();
            if (section != 1000)
                throw std::runtime_error(std::format("Expected SPT section 1000, got {}", section));
            file->mVersionId = stream.readIntString();
            if (file->mVersionId != "__IdvSpt_02_")
                throw std::runtime_error(std::format("Unsupported SPT version '{}'", file->mVersionId));
        }

        {
            int32_t section = stream.readSection();
            if (section != 1002)
                throw std::runtime_error(std::format("Expected SPT section 1002, got {}", section));
        }

        while (true)
        {
            int32_t section = stream.readSection();
            switch (section)
            {
                case 2000:
                    file->mTextureMap = stream.readIntString();
                    break;
                case 2001:
                    file->mLodFarDistance = stream.readFloat();
                    break;
                case 2002:
                    file->mUnknown2002 = stream.readByte();
                    break;
                case 2003:
                    file->mLodNearDistance = stream.readFloat();
                    break;
                case 2005:
                    file->mSeed = stream.readInt();
                    break;
                case 2006:
                    file->mSize = stream.readFloat();
                    break;
                case 2007:
                    file->mSizeVariance = stream.readFloat();
                    break;
                case 1014:
                    file->mNumLevels = stream.readInt();
                    break;
                case 1015:
                    break;
                case 1016: // begin bezier spline block for one level
                {
                    SPTLevelData level;
                    level.read(stream);
                    file->mLevelData.push_back(std::move(level));
                    break;
                }
                case 1003:
                    return file;
                default:
                    throw std::runtime_error(std::format("Unsupported section {} in SPT file", section));
            }
        }
    }
}
