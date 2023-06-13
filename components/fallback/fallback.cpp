#include "fallback.hpp"

#include <string>
#include <vector>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/conversion.hpp>

#include "validate.hpp"

namespace Fallback
{
    std::map<std::string, int, std::less<>> Map::mIntFallbackMap;
    std::map<std::string, float, std::less<>> Map::mFloatFallbackMap;
    std::map<std::string, std::string, std::less<>> Map::mNonNumericFallbackMap;

    void Map::init(const std::map<std::string, std::string>& fallback)
    {
        for (const auto& [key, value] : fallback)
        {
            if (isAllowedIntFallbackKey(key))
                mIntFallbackMap.emplace(key, Misc::StringUtils::toNumeric<int>(value, 0));
            else if (isAllowedFloatFallbackKey(key))
                mFloatFallbackMap.emplace(key, Misc::StringUtils::toNumeric<float>(value, 0.0f));
            else if (isAllowedNonNumericFallbackKey(key))
                mNonNumericFallbackMap.emplace(key, value);
            else if (!isAllowedUnusedFallbackKey(key))
                Log(Debug::Error) << "Ignoring unknown fallback: " << key;
        }
    }

    std::string_view Map::getString(std::string_view fall)
    {
        const auto it = mNonNumericFallbackMap.find(fall);
        if (it == mNonNumericFallbackMap.end())
        {
            if (!isAllowedNonNumericFallbackKey(fall))
                throw std::logic_error("Requested invalid string fallback: " + std::string(fall));
            return {};
        }
        return it->second;
    }

    float Map::getFloat(std::string_view fall)
    {
        const auto it = mFloatFallbackMap.find(fall);
        if (it == mFloatFallbackMap.end())
        {
            if (!isAllowedFloatFallbackKey(fall))
                throw std::logic_error("Requested invalid float fallback: " + std::string(fall));
            return {};
        }
        return it->second;
    }

    int Map::getInt(std::string_view fall)
    {
        const auto it = mIntFallbackMap.find(fall);
        if (it == mIntFallbackMap.end())
        {
            if (!isAllowedIntFallbackKey(fall))
                throw std::logic_error("Requested invalid int fallback: " + std::string(fall));
            return {};
        }
        return it->second;
    }

    bool Map::getBool(std::string_view fall)
    {
        return getInt(fall) != 0;
    }

    osg::Vec4f Map::getColour(std::string_view fall)
    {
        const std::string_view sum = getString(fall);

        if (!sum.empty())
        {
            std::vector<std::string> ret;
            Misc::StringUtils::split(sum, ret, ",");

            if (ret.size() == 3)
            {
                const auto r = Misc::StringUtils::toNumeric<float>(ret[0]);
                const auto g = Misc::StringUtils::toNumeric<float>(ret[1]);
                const auto b = Misc::StringUtils::toNumeric<float>(ret[2]);

                if (r.has_value() && g.has_value() && b.has_value())
                {
                    return osg::Vec4f(*r / 255.0f, *g / 255.0f, *b / 255.0f, 1.0f);
                }
            }

            Log(Debug::Error) << "Error: '" << fall << "' setting value (" << sum
                              << ") is not a valid color, using middle gray as a fallback";
        }

        return osg::Vec4f(0.5f, 0.5f, 0.5f, 1.f);
    }
}
