#include "file.hpp"

#include <format>
#include <stdexcept>

#include "stream.hpp"

namespace Spt
{
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
                case 5000:
                    stream.skip(12);
                    break;
                case 5001:
                    stream.skip(12);
                    break;
                case 5002:
                    stream.skip(12);
                    break;
                case 5003:
                    stream.skip(12);
                    break;
                case 5004:
                    stream.skip(12);
                    break;
                case 5005:
                    stream.skip(4);
                    break;
                case 5006:
                    stream.skip(1);
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unknown section {} in discarded block ending at {}", section, endSection));
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
            int32_t section;
            try
            {
                section = stream.readSection();
            }
            catch (const SPTEOFError&)
            {
                break;
            }
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
                case 19000:
                    file->mShadowRelated.read(stream);
                    break;
                case 20000:
                    file->mCompositeMapGlobal.read(stream);
                    break;
                case 21000:
                    file->mLeafRockAngleScalar = stream.readFloat();
                    break;
                case 21001:
                    file->mLeafRustleAngleScalar = stream.readFloat();
                    break;
                case 22000:
                    file->mBranchWeightingFlexibility = stream.readByte();
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unknown top-level section {} in SPT file", section));
            }
        }
        return file;
    }
}