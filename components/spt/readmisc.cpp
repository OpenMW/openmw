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

    bool SPTFlaresEntry::readField(SPTStream& stream, int32_t section)
    {
        switch (section)
        {
            case 16002:
                stream.read(mSegmentsPack);
                return true;
            case 16003:
                stream.read(mFlaresNumber);
                return true;
            case 16004:
                stream.read(mFlaresBalance);
                return true;
            case 16005:
                stream.read(mRadialInfluence);
                return true;
            case 16006:
                stream.read(mRadialInfluenceVariance);
                return true;
            case 16007:
                stream.read(mRadialExponent);
                return true;
            case 16008:
                stream.read(mRadialDistance);
                return true;
            case 16009:
                stream.read(mRadialDistanceVariance);
                return true;
            case 16010:
                stream.read(mLengthDistance);
                return true;
            case 16011:
                stream.read(mLengthDistanceVariance);
                return true;
            case 16012:
                stream.read(mLengthExponent);
                return true;
            default:
                return false;
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
            if (!entry.readField(stream, section))
                throw std::runtime_error(std::format("Unknown section {} in flares block", section));
            if (section == 16012)
            {
                mEntries.push_back(std::move(entry));
                entry = {};
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

    void SPTUserData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 19001)
                return;
            switch (section)
            {
                case 19002:
                    mText = stream.readIntString();
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in user data block", section));
            }
        }
    }
}
