#include "regionbindings.hpp"
#include "recordstore.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>

#include <components/esm3/loadregn.hpp>
#include <components/esm3/loadsoun.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/color.hpp>

#include "../mwworld/weather.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::Region> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::Region::SoundRef> : std::false_type
    {
    };
}

namespace
{
    std::span<const uint8_t> getRegionWeatherChances(const ESM::Region& rec)
    {
        const auto chances = MWBase::Environment::get().getWorld()->getRegionWeatherChances(rec.mId);
        if (!chances.empty())
            return chances;
        return rec.mData.mProbabilities;
    }
}

namespace MWLua
{
    sol::table initCoreRegionBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table regions(lua, sol::create);
        addRecordFunctionBinding<ESM::Region>(regions, context);
        auto soundRefT = lua.new_usertype<ESM::Region::SoundRef>("ESM3_RegionSoundRef");

        soundRefT[sol::meta_function::to_string] = [](const ESM::Region::SoundRef& ref) -> std::string {
            return "ESM3_RegionSoundRef[" + ref.mSound.toDebugString() + "]";
        };
        soundRefT["soundId"]
            = sol::readonly_property([](const ESM::Region::SoundRef& ref) { return ref.mSound.serializeText(); });

        soundRefT["chance"] = sol::readonly_property([](const ESM::Region::SoundRef& ref) { return ref.mChance; });

        // Region record
        auto regionT = lua.new_usertype<ESM::Region>("ESM3_Region");
        regionT[sol::meta_function::to_string]
            = [](const ESM::Region& rec) -> std::string { return "ESM3_Region[" + rec.mId.toDebugString() + "]"; };
        regionT["id"] = sol::readonly_property([](const ESM::Region& rec) { return rec.mId.serializeText(); });
        regionT["name"] = sol::readonly_property([](const ESM::Region& rec) -> std::string_view { return rec.mName; });
        regionT["mapColor"] = sol::readonly_property(
            [](const ESM::Region& rec) -> Misc::Color { return Misc::Color::fromRGB(rec.mMapColor); });
        regionT["sleepList"]
            = sol::readonly_property([](const ESM::Region& rec) -> ESM::RefId { return rec.mSleepList; });

        regionT["weatherProbabilities"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Region& rec) {
            sol::table res(lua, sol::create);
            const auto chances = getRegionWeatherChances(rec);
            for (size_t i = 0; i < chances.size(); ++i)
            {
                const MWWorld::Weather* weather = MWBase::Environment::get().getWorld()->getWeather(i);
                if (weather != nullptr)
                    res[weather->mId.serializeText()] = chances[i];
            }
            return LuaUtil::makeReadOnly(res);
        });
        regionT["setProbability"] = [](const ESM::Region& rec, std::string_view weatherId, int value) {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            ESM::RefId id = ESM::RefId::deserializeText(weatherId);
            const auto& allWeather = world->getAllWeather();
            auto it = std::find_if(allWeather.begin(), allWeather.end(),
                [id](const MWWorld::Weather& weather) { return weather.mId == id; });
            if (it == allWeather.end())
                throw std::runtime_error("Weather \"" + std::string(weatherId) + "\" not found");

            const auto current = getRegionWeatherChances(rec);
            std::vector<uint8_t> chances(current.begin(), current.end());
            const size_t index = std::distance(allWeather.begin(), it);
            if (chances.size() <= index)
                chances.resize(index + 1, 0);
            chances[index] = static_cast<uint8_t>(std::clamp(value, 0, 100));
            world->modRegion(rec.mId, chances);
        };
        auto resetProbability = [](const ESM::Region& rec) {
            MWBase::Environment::get().getWorld()->modRegion(rec.mId, rec.mData.mProbabilities);
        };
        regionT["resetProbability"] = resetProbability;
        regionT["sounds"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Region& rec) {
            sol::table res(lua, sol::create);
            for (const auto& soundRef : rec.mSoundList)
                res.add(soundRef);
            return LuaUtil::makeReadOnly(res);
        });
        return LuaUtil::makeReadOnly(regions);
    }
}
