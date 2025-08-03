#include "regionbindings.hpp"
#include "recordstore.hpp"

#include <components/esm3/loadregn.hpp>
#include <components/esm3/loadsoun.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/color.hpp>

#include <sol/sol.hpp>
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
            = sol::readonly_property([](const ESM::Region& rec) { return LuaUtil::serializeRefId(rec.mSleepList); });

        regionT["weatherProbabilities"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Region& rec) {
            constexpr std::array<const char*, 10> WeatherNames
                = { "clear", "cloudy", "foggy", "overcast", "rain", "thunder", "ash", "blight", "snow", "blizzard" };

            sol::table res(lua, sol::create);
            for (size_t i = 0; i < rec.mData.mProbabilities.size(); ++i)
            {
                res[LuaUtil::toLuaIndex(i)] = rec.mData.mProbabilities[i];
                res[WeatherNames[i]] = rec.mData.mProbabilities[i];
            }
            return res;
        });
        regionT["sounds"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Region& rec) {
            sol::table res(lua, sol::create);
            for (const auto& soundRef : rec.mSoundList)
                res.add(soundRef);
            return LuaUtil::makeReadOnly(res);
        });
        return LuaUtil::makeReadOnly(regions);
    }
}
