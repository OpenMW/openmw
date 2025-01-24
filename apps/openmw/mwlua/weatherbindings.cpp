#include "weatherbindings.hpp"

#include <components/esm3/loadregn.hpp>
#include <components/lua/util.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/weather.hpp"

namespace
{
    class WeatherStore
    {
    public:
        const MWWorld::Weather* get(size_t index) const
        {
            return MWBase::Environment::get().getWorld()->getWeather(index);
        }
        const MWWorld::Weather* get(const ESM::RefId& id) const
        {
            return MWBase::Environment::get().getWorld()->getWeather(id);
        }
        size_t size() const { return MWBase::Environment::get().getWorld()->getAllWeather().size(); }
    };
}

namespace MWLua
{
    sol::table initCoreWeatherBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        auto weatherT = lua.new_usertype<MWWorld::Weather>("Weather");
        weatherT[sol::meta_function::to_string]
            = [](const MWWorld::Weather& w) -> std::string { return "Weather[" + w.mName + "]"; };
        weatherT["name"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mName; });
        weatherT["windSpeed"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mWindSpeed; });
        weatherT["cloudSpeed"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mCloudSpeed; });
        weatherT["cloudTexture"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mCloudTexture; });
        weatherT["cloudsMaximumPercent"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mCloudsMaximumPercent; });
        weatherT["isStorm"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mIsStorm; });
        weatherT["stormDirection"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mStormDirection; });
        weatherT["glareView"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mGlareView; });
        weatherT["rainSpeed"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainSpeed; });
        weatherT["rainEntranceSpeed"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainEntranceSpeed; });
        weatherT["rainEffect"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainEffect; });
        weatherT["rainMaxRaindrops"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainMaxRaindrops; });
        weatherT["rainDiameter"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainDiameter; });
        weatherT["rainThreshold"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainThreshold; });
        weatherT["rainMaxHeight"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainMaxHeight; });
        weatherT["rainMinHeight"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainMinHeight; });
        weatherT["rainLoopSoundID"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainLoopSoundID.serializeText(); });
        weatherT["thunderSoundID"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            for (const auto& soundId : w.mThunderSoundID)
                result.add(soundId.serializeText());
            return result;
        });
        weatherT["sunDiscSunsetColor"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mSunDiscSunsetColor; });
        weatherT["ambientLoopSoundID"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mAmbientLoopSoundID.serializeText(); });
        weatherT["ambientColor"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = w.mAmbientColor.getSunriseValue();
            result["day"] = w.mAmbientColor.getDayValue();
            result["sunset"] = w.mAmbientColor.getSunsetValue();
            result["night"] = w.mAmbientColor.getNightValue();
            return result;
        });
        weatherT["fogColor"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = w.mFogColor.getSunriseValue();
            result["day"] = w.mFogColor.getDayValue();
            result["sunset"] = w.mFogColor.getSunsetValue();
            result["night"] = w.mFogColor.getNightValue();
            return result;
        });
        weatherT["skyColor"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = w.mSkyColor.getSunriseValue();
            result["day"] = w.mSkyColor.getDayValue();
            result["sunset"] = w.mSkyColor.getSunsetValue();
            result["night"] = w.mSkyColor.getNightValue();
            return result;
        });
        weatherT["sunColor"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = w.mSunColor.getSunriseValue();
            result["day"] = w.mSunColor.getDayValue();
            result["sunset"] = w.mSunColor.getSunsetValue();
            result["night"] = w.mSunColor.getNightValue();
            return result;
        });
        weatherT["landFogDepth"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = w.mLandFogDepth.getSunriseValue();
            result["day"] = w.mLandFogDepth.getDayValue();
            result["sunset"] = w.mLandFogDepth.getSunsetValue();
            result["night"] = w.mLandFogDepth.getNightValue();
            return result;
        });
        weatherT["particleEffect"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mParticleEffect; });
        weatherT["distantLandFogFactor"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mDL.FogFactor; });
        weatherT["distantLandFogOffset"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mDL.FogOffset; });
        weatherT["scriptId"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mScriptId; });
        weatherT["recordId"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mId.serializeText(); });

        api["getCurrent"] = []() { return MWBase::Environment::get().getWorld()->getCurrentWeather(); };
        api["getNext"] = []() -> sol::optional<const MWWorld::Weather&> {
            auto next = MWBase::Environment::get().getWorld()->getNextWeather();
            if (next != nullptr)
                return *next;
            return sol::nullopt;
        };
        api["getTransition"] = []() { return MWBase::Environment::get().getWorld()->getWeatherTransition(); };

        api["changeWeather"] = [](std::string_view regionId, const MWWorld::Weather& weather) {
            ESM::RefId region = ESM::RefId::deserializeText(regionId);
            const ESM::Region* reg = MWBase::Environment::get().getESMStore()->get<ESM::Region>().search(region);
            if (reg)
                MWBase::Environment::get().getWorld()->changeWeather(region, weather.mId);
            else
                throw std::runtime_error("Region not found");
        };

        sol::usertype<WeatherStore> storeT = lua.new_usertype<WeatherStore>("WeatherWorldStore");
        storeT[sol::meta_function::to_string]
            = [](const WeatherStore& store) { return "{" + std::to_string(store.size()) + " Weather records}"; };
        storeT[sol::meta_function::length] = [](const WeatherStore& store) { return store.size(); };
        storeT[sol::meta_function::index] = sol::overload(
            [](const WeatherStore& store, size_t index) -> const MWWorld::Weather* {
                return store.get(LuaUtil::fromLuaIndex(index));
            },
            [](const WeatherStore& store, std::string_view id) -> const MWWorld::Weather* {
                return store.get(ESM::RefId::deserializeText(id));
            });
        storeT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
        storeT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

        // Provide access to the store.
        api["records"] = WeatherStore{};

        api["getCurrentSunVisibility"] = []() { return MWBase::Environment::get().getWorld()->getSunVisibility(); };
        api["getCurrentSunPercentage"] = []() { return MWBase::Environment::get().getWorld()->getSunPercentage(); };
        api["getCurrentWindSpeed"] = []() { return MWBase::Environment::get().getWorld()->getWindSpeed(); };
        api["getCurrentStormDirection"] = []() { return MWBase::Environment::get().getWorld()->getStormDirection(); };

        return LuaUtil::makeReadOnly(api);
    }
}
