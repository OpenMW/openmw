#include "file.hpp"

#include <format>
#include <stdexcept>

#include "stream.hpp"

namespace Spt
{
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
                    stream.read(obj.mPosition);
                    stream.read(obj.mRadius);
                    mObjects.push_back(std::move(obj));
                    break;
                }
                case 12003:
                {
                    SPTCollisionObject obj;
                    obj.mType = SPTCollisionType::Capsule;
                    stream.read(obj.mPosition);
                    stream.read(obj.mRadius);
                    stream.read(obj.mLength);
                    mObjects.push_back(std::move(obj));
                    break;
                }
                case 12004:
                {
                    SPTCollisionObject obj;
                    obj.mType = SPTCollisionType::Box;
                    stream.read(obj.mBoxMin);
                    stream.read(obj.mBoxMax);
                    mObjects.push_back(std::move(obj));
                    break;
                }
                default:
                    throw std::runtime_error(std::format("Unknown section {} in collision block", section));
            }
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
                case 16002:
                    stream.read(entry.mSegmentsPack);
                    break;
                case 16003:
                    stream.read(entry.mFlaresNumber);
                    break;
                case 16004:
                    stream.read(entry.mFlaresBalance);
                    break;
                case 16005:
                    stream.read(entry.mRadialInfluence);
                    break;
                case 16006:
                    stream.read(entry.mRadialInfluenceVariance);
                    break;
                case 16007:
                    stream.read(entry.mRadialExponent);
                    break;
                case 16008:
                    stream.read(entry.mRadialDistance);
                    break;
                case 16009:
                    stream.read(entry.mRadialDistanceVariance);
                    break;
                case 16010:
                    stream.read(entry.mLengthDistance);
                    break;
                case 16011:
                    stream.read(entry.mLengthDistanceVariance);
                    break;
                case 16012:
                    stream.read(entry.mLengthExponent);
                    mEntries.push_back(std::move(entry));
                    entry = {};
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in flares block", section));
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
                    stream.read(mUnknown18002);
                    break;
                case 18003:
                    stream.read(mUnknown18003);
                    break;
                case 18004:
                    stream.read(mUnknown18004);
                    break;
                case 18005:
                    mSelfShadowMap = stream.readIntString();
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in shadow block", section));
            }
        }
    }

    void SPTShadowRelatedData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 19001)
                return;
            switch (section)
            {
                case 19002:
                    stream.read(mUnknown19002);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in shadow related block", section));
            }
        }
    }
}