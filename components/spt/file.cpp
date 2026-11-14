#include "file.hpp"

#include <format>
#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include "stream.hpp"

namespace Spt
{
    namespace
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
                        stream.read(header.mLodFarDistance);
                        break;
                    case 2002:
                        stream.read(header.mUnknown2002);
                        break;
                    case 2003:
                        stream.read(header.mLodNearDistance);
                        break;
                    case 2004:
                        stream.read(header.mUnknown2004);
                        break;
                    case 2005:
                        stream.read(header.mSeed);
                        break;
                    case 2006:
                        stream.read(header.mSize);
                        break;
                    case 2007:
                        stream.read(header.mSizeVariance);
                        break;
                    case 1014:
                        stream.read(header.mNumLevels);
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

    SPTFilePtr parse(Files::IStreamPtr&& inputStream, std::string_view name)
    {
        auto file = std::make_shared<SPTFile>();
        SPTStream stream(std::move(inputStream));

        if (stream.eof())
            throw std::runtime_error("Not an SPT file: empty");
        stream.expectSection(1000);
        file->mHeader.read(stream);

        while (!stream.eof())
        {
            const int32_t section = stream.readSection();
            switch (section)
            {
                case 7000:
                    file->mLeafLods.read(stream);
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
                    stream.read(file->mFlares.mFlareSeed);
                    break;
                case 16014:
                    stream.read(file->mLod.mLeafTransition);
                    break;
                case 18000:
                    file->mShadow.read(stream);
                    break;
                case 19000:
                    file->mUserData.read(stream);
                    break;
                case 20000:
                    file->mCompositeMapGlobal.read(stream);
                    break;
                case 21000:
                    stream.read(file->mLeafRockAngleScalar);
                    break;
                case 21001:
                    stream.read(file->mLeafRustleAngleScalar);
                    break;
                case 22000:
                    stream.read(file->mBranchWeightingFlexibility);
                    break;
                case 23000:
                    file->mLightSeam.read(stream);
                    break;
                case 24000:
                    file->mLeafPlacement.read(stream);
                    break;
                case 25000:
                    file->mSupplementalFrond.read(stream);
                    break;
                case 26000:
                    file->mSupplementalBranch.read(stream);
                    break;
                case 27000:
                    file->mFloor.read(stream);
                    break;
                case 28000:
                    file->mLeafSmoothing.read(stream);
                    break;
                case 29000:
                    stream.expectSection(29002);
                    stream.read(file->mClusterFirstVisibleLevel);
                    stream.expectSection(29001);
                    break;
                case 30000:
                    file->mStandardShader.read(stream);
                    break;
                case 40000:
                    file->mRoots.read(stream);
                    break;
                case 50000:
                    file->mTexCoords.read(stream);
                    break;
                case 60000:
                    file->mMapBank.read(stream);
                    break;
                case 71000:
                    file->mMeshInfo.read(stream);
                    break;
                case 72000:
                    file->mLeafMeshes.read(stream);
                    break;
                case 73000:
                    file->mCollisionExtra.read(stream);
                    break;
                case 74000:
                    stream.expectSection(74002);
                    stream.read(file->mOrientationAngle);
                    stream.expectSection(74001);
                    break;
                case 75000:
                    file->mLodFade.read(stream);
                    break;
                default:
                    if (section < 22000)
                        throw std::runtime_error(std::format("Unknown top-level section {} in SPT file", section));
                    // Newer than anything 4.2 writes, and nothing we read can follow it
                    Log(Debug::Verbose) << "Ignoring SPT sections from " << section << " onwards in " << name;
                    return file;
            }
        }
        return file;
    }
}
