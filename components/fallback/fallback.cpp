#include "fallback.hpp"

#include <sstream>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/conversion.hpp>

namespace Fallback
{
    std::map<std::string, std::string, std::less<>> Map::mFallbackMap;

    void Map::init(const std::map<std::string, std::string>& fallback)
    {
        for (const auto& entry : fallback)
            mFallbackMap.insert(entry);
    }

    std::string_view Map::getString(std::string_view fall)
    {
        auto it = mFallbackMap.find(fall);
        if (it == mFallbackMap.end())
        {
            return {};
        }
        return it->second;
    }

    float Map::getFloat(std::string_view fall)
    {
        std::string_view fallback = getString(fall);
        return Misc::StringUtils::toNumeric<float>(fallback, 0.0f);
    }

    int Map::getInt(std::string_view fall)
    {
        std::string_view fallback = getString(fall);
        return Misc::StringUtils::toNumeric<int>(fallback, 0);
    }

    bool Map::getBool(std::string_view fall)
    {
        std::string_view fallback = getString(fall);
        return !fallback.empty() && fallback != "0";
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

                if (r && g && b)
                {
                    return { r.value() / 255.0f, g.value() / 255.0f, b.value() / 255.0f, 1.0f };
                }
            }

            Log(Debug::Error) << "Error: '" << fall << "' setting value (" << sum
                              << ") is not a valid color, using middle gray as a fallback";
        }

        return { 0.5f, 0.5f, 0.5f, 1.f };
    }
}
