#include "weatherbindings.hpp"

#include <osg/Vec4f>

#include <components/esm3/loadregn.hpp>
#include <components/lua/util.hpp>
#include <components/misc/color.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/weather.hpp"

#include "context.hpp"

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

    Misc::Color color(const osg::Vec4f& color)
    {
        return Misc::Color(color.r(), color.g(), color.b(), color.a());
    }
}

namespace MWLua
{
    sol::table initCoreWeatherBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        auto weatherT = lua.new_usertype<MWWorld::Weather>("Weather");
        weatherT[sol::meta_function::to_string]
            = [](const MWWorld::Weather& w) -> std::string { return "Weather[" + w.mName + "]"; };
        weatherT["name"]
            = sol::readonly_property([](const MWWorld::Weather& w) -> std::string_view { return w.mName; });
        weatherT["windSpeed"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mWindSpeed; });
        weatherT["cloudSpeed"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mCloudSpeed; });
        weatherT["cloudTexture"] = sol::readonly_property([vfs](const MWWorld::Weather& w) {
            return Misc::ResourceHelpers::correctTexturePath(w.mCloudTexture, vfs);
        });
        weatherT["cloudsMaximumPercent"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mCloudsMaximumPercent; });
        weatherT["isStorm"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mIsStorm; });
        weatherT["stormDirection"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mStormDirection; });
        weatherT["glareView"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mGlareView; });
        weatherT["rainSpeed"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainSpeed; });
        weatherT["rainEntranceSpeed"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mRainEntranceSpeed; });
        weatherT["rainEffect"] = sol::readonly_property([](const MWWorld::Weather& w) -> sol::optional<std::string> {
            if (w.mRainEffect.empty())
                return sol::nullopt;
            return w.mRainEffect;
        });
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
            = sol::readonly_property([](const MWWorld::Weather& w) { return color(w.mSunDiscSunsetColor); });
        weatherT["ambientLoopSoundID"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mAmbientLoopSoundID.serializeText(); });
        weatherT["ambientColor"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = color(w.mAmbientColor.getSunriseValue());
            result["day"] = color(w.mAmbientColor.getDayValue());
            result["sunset"] = color(w.mAmbientColor.getSunsetValue());
            result["night"] = color(w.mAmbientColor.getNightValue());
            return result;
        });
        weatherT["fogColor"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = color(w.mFogColor.getSunriseValue());
            result["day"] = color(w.mFogColor.getDayValue());
            result["sunset"] = color(w.mFogColor.getSunsetValue());
            result["night"] = color(w.mFogColor.getNightValue());
            return result;
        });
        weatherT["skyColor"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = color(w.mSkyColor.getSunriseValue());
            result["day"] = color(w.mSkyColor.getDayValue());
            result["sunset"] = color(w.mSkyColor.getSunsetValue());
            result["night"] = color(w.mSkyColor.getNightValue());
            return result;
        });
        weatherT["sunColor"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            result["sunrise"] = color(w.mSunColor.getSunriseValue());
            result["day"] = color(w.mSunColor.getDayValue());
            result["sunset"] = color(w.mSunColor.getSunsetValue());
            result["night"] = color(w.mSunColor.getNightValue());
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
            = sol::readonly_property([](const MWWorld::Weather& w) -> sol::optional<std::string> {
                  if (w.mParticleEffect.empty())
                      return sol::nullopt;
                  return w.mParticleEffect;
              });
        weatherT["distantLandFogFactor"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mDL.FogFactor; });
        weatherT["distantLandFogOffset"]
            = sol::readonly_property([](const MWWorld::Weather& w) { return w.mDL.FogOffset; });
        weatherT["scriptId"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mScriptId; });
        weatherT["recordId"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mId.serializeText(); });

        api["getCurrent"]
            = []() -> const MWWorld::Weather* { return &MWBase::Environment::get().getWorld()->getCurrentWeather(); };
        api["getNext"]
            = []() -> const MWWorld::Weather* { return MWBase::Environment::get().getWorld()->getNextWeather(); };
        api["getTransition"] = []() { return MWBase::Environment::get().getWorld()->getWeatherTransition(); };

        api["changeWeather"] = [](std::string_view regionId, const MWWorld::Weather& weather) {
            ESM::RefId region = ESM::RefId::deserializeText(regionId);
            MWBase::Environment::get().getESMStore()->get<ESM::Region>().find(region);
            MWBase::Environment::get().getWorld()->changeWeather(region, weather.mId);
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

        api["getCurrentSunLightDirection"] = []() {
            osg::Vec4f sunPos = MWBase::Environment::get().getWorld()->getSunLightPosition();
            // normalize to get the direction towards the sun
            sunPos.normalize();

            // and invert it to get the direction of the sun light
            return -sunPos;
        };
        api["getCurrentSunVisibility"] = []() { return MWBase::Environment::get().getWorld()->getSunVisibility(); };
        api["getCurrentSunPercentage"] = []() { return MWBase::Environment::get().getWorld()->getSunPercentage(); };
        api["getCurrentWindSpeed"] = []() { return MWBase::Environment::get().getWorld()->getWindSpeed(); };
        api["getCurrentStormDirection"] = []() { return MWBase::Environment::get().getWorld()->getStormDirection(); };

        return LuaUtil::makeReadOnly(api);
    }
}
