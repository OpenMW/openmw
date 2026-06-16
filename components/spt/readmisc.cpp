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
                    entry.mSegmentsPack = stream.readFloat();
                    break;
                case 16003:
                    entry.mFlaresNumber = stream.readInt();
                    break;
                case 16004:
                    entry.mFlaresBalance = stream.readFloat();
                    break;
                case 16005:
                    entry.mRadialInfluence = stream.readFloat();
                    break;
                case 16006:
                    entry.mRadialInfluenceVariance = stream.readFloat();
                    break;
                case 16007:
                    entry.mRadialExponent = stream.readFloat();
                    break;
                case 16008:
                    entry.mRadialDistance = stream.readFloat();
                    break;
                case 16009:
                    entry.mRadialDistanceVariance = stream.readFloat();
                    break;
                case 16010:
                    entry.mLengthDistance = stream.readFloat();
                    break;
                case 16011:
                    entry.mLengthDistanceVariance = stream.readFloat();
                    break;
                case 16012:
                    entry.mLengthExponent = stream.readFloat();
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
                    mUnknown19002 = stream.readInt();
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in shadow related block", section));
            }
        }
    }
}