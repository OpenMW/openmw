#include "regionbindings.hpp"
#include "recordstore.hpp"

#include <components/esm3/loadregn.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/color.hpp>

#include "idcollectionbindings.hpp"

namespace
{
    struct RegionSoundRef
    {
        ESM::RefId mSoundId;
        uint8_t mChance;

        RegionSoundRef(const ESM::Region::SoundRef& ref)
            : mSoundId(ref.mSound)
            , mChance(ref.mChance)
        {
        }
    };
}

namespace sol
{
    template <>
    struct is_automagical<ESM::Region> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWWorld::Store<RegionSoundRef>> : std::false_type
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

        // Region record
        auto regionT = lua.new_usertype<ESM::Region>("ESM3_Region");
        regionT[sol::meta_function::to_string]
            = [](const ESM::Region& rec) -> std::string { return "ESM3_Region[" + rec.mId.toDebugString() + "]"; };
        regionT["id"] = sol::readonly_property([](const ESM::Region& rec) { return rec.mId.serializeText(); });
        regionT["name"] = sol::readonly_property([](const ESM::Region& rec) -> std::string_view { return rec.mName; });
        regionT["mapColor"] = sol::readonly_property([](const ESM::Region& rec) -> Misc::Color {
            uint32_t c = rec.mMapColor;
            float r = ((c >> 16) & 0xFF) / 255.f;
            float g = ((c >> 8) & 0xFF) / 255.f;
            float b = (c & 0xFF) / 255.f;
            return Misc::Color(r, g, b, 1.f); // assume alpha = 1.0
        });
        regionT["sleepList"]
            = sol::readonly_property([](const ESM::Region& rec) { return rec.mSleepList.serializeText(); });

        regionT["weatherProbabilities"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Region& rec) {
            static const std::array<const char*, 10> WeatherNames
                = { "clear", "cloudy", "foggy", "overcast", "rain", "thunder", "ash", "blight", "snow", "blizzard" };

            sol::table res(lua, sol::create);
            for (size_t i = 0; i < rec.mData.mProbabilities.size(); ++i)
            {
                res[i + 1] = rec.mData.mProbabilities[i]; // Numeric index (Lua-style 1-based)
                res[WeatherNames[i]] = rec.mData.mProbabilities[i]; // Named index
            }
            return res;
        });

        regionT["sounds"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Region& rec) {
            sol::table res(lua, sol::create);
            for (const auto& soundRef : rec.mSoundList)
                res.add(RegionSoundRef(soundRef));
            return res;
        });

        auto soundRefT = lua.new_usertype<RegionSoundRef>("ESM3_RegionSoundRef");
        soundRefT[sol::meta_function::to_string] = [](const RegionSoundRef& ref) -> std::string {
            return "ESM3_RegionSoundRef[" + ref.mSoundId.toDebugString() + "]";
        };
        soundRefT["soundId"]
            = sol::readonly_property([](const RegionSoundRef& ref) { return ref.mSoundId.serializeText(); });
        soundRefT["chance"] = sol::readonly_property([](const RegionSoundRef& ref) { return ref.mChance; });

        return LuaUtil::makeReadOnly(regions);
    }
}
