#include "weatherbindings.hpp"

#include <type_traits>

#include <osg/Vec4f>

#include <components/esm3/loadregn.hpp>
#include <components/lua/util.hpp>
#include <components/misc/color.hpp>
#include <components/misc/finitevalues.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/scene.hpp"
#include "../mwworld/weather.hpp"

#include "context.hpp"
#include "object.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWWorld::TimeOfDayInterpolator<float>> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWWorld::TimeOfDayInterpolator<osg::Vec4f>> : std::false_type
    {
    };
}

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

    template <class Cell>
    bool hasWeather(const Cell& cell, bool requireExterior)
    {
        if (requireExterior && !cell.mStore->isQuasiExterior() && !cell.mStore->isExterior())
            return false;
        return MWBase::Environment::get().getWorldScene()->isCellActive(*cell.mStore);
    }

    template <class Getter>
    auto overloadForActiveCell(Getter&& getter, bool requireExterior = true)
    {
        using Result = std::invoke_result_t<Getter>;
        return sol::overload(
            [=](const MWLua::GCell& cell) -> Result {
                if (!hasWeather(cell, requireExterior))
                    return Result{};
                return getter();
            },
            [=](const MWLua::LCell& cell) -> Result {
                if (!hasWeather(cell, requireExterior))
                    return Result{};
                return getter();
            });
    }

    template <class T>
    using WeatherGetter = T (MWBase::World::*)() const;

    template <class T>
    auto overloadWeatherGetter(WeatherGetter<T> getter)
    {
        return overloadForActiveCell([=]() -> std::optional<T> {
            const MWBase::World& world = *MWBase::Environment::get().getWorld();
            return (world.*getter)();
        });
    }

    void createFloatInterpolator(sol::state_view lua)
    {
        using Misc::FiniteFloat;
        using T = MWWorld::TimeOfDayInterpolator<float>;

        auto interT = lua.new_usertype<T>("TimeOfDayInterpolatorFloat");

        interT["sunrise"] = sol::property([](const T& inter) { return inter.getSunriseValue(); },
            [](T& inter, FiniteFloat value) { inter.setSunriseValue(value); });
        interT["sunset"] = sol::property([](const T& inter) { return inter.getSunsetValue(); },
            [](T& inter, FiniteFloat value) { inter.setSunsetValue(value); });
        interT["day"] = sol::property([](const T& inter) { return inter.getDayValue(); },
            [](T& inter, FiniteFloat value) { inter.setDayValue(value); });
        interT["night"] = sol::property([](const T& inter) { return inter.getNightValue(); },
            [](T& inter, FiniteFloat value) { inter.setNightValue(value); });
    }

    void createColorInterpolator(sol::state_view lua)
    {
        using Misc::Color;
        using T = MWWorld::TimeOfDayInterpolator<osg::Vec4f>;

        auto interT = lua.new_usertype<T>("TimeOfDayInterpolatorColor");

        interT["sunrise"] = sol::property([](const T& inter) { return Color::fromVec(inter.getSunriseValue()); },
            [](T& inter, const Color& value) { inter.setSunriseValue(value.toVec()); });
        interT["sunset"] = sol::property([](const T& inter) { return Color::fromVec(inter.getSunsetValue()); },
            [](T& inter, const Color& value) { inter.setSunsetValue(value.toVec()); });
        interT["day"] = sol::property([](const T& inter) { return Color::fromVec(inter.getDayValue()); },
            [](T& inter, const Color& value) { inter.setDayValue(value.toVec()); });
        interT["night"] = sol::property([](const T& inter) { return Color::fromVec(inter.getNightValue()); },
            [](T& inter, const Color& value) { inter.setNightValue(value.toVec()); });
    }
}

namespace MWLua
{
    sol::table initCoreWeatherBindings(const Context& context)
    {
        using Misc::FiniteFloat;
        using Misc::FiniteVec3f;

        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        auto weatherT = lua.new_usertype<MWWorld::Weather>("Weather");
        createFloatInterpolator(lua);
        createColorInterpolator(lua);

        weatherT[sol::meta_function::to_string]
            = [](const MWWorld::Weather& w) -> std::string { return "Weather[" + w.mName + "]"; };
        weatherT["name"]
            = sol::readonly_property([](const MWWorld::Weather& w) -> std::string_view { return w.mName; });
        weatherT["scriptId"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mScriptId; });
        weatherT["recordId"] = sol::readonly_property([](const MWWorld::Weather& w) { return w.mId.serializeText(); });
        weatherT["thunderSoundID"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
            sol::table result(lua, sol::create);
            for (const auto& soundId : w.mThunderSoundID)
                result.add(soundId.serializeText());
            return result;
        });

        weatherT["windSpeed"] = sol::property([](const MWWorld::Weather& w) { return w.mWindSpeed; },
            [](MWWorld::Weather& w, const FiniteFloat windSpeed) { w.mWindSpeed = windSpeed; });
        weatherT["cloudSpeed"] = sol::property([](const MWWorld::Weather& w) { return w.mCloudSpeed; },
            [](MWWorld::Weather& w, const FiniteFloat cloudSpeed) { w.mCloudSpeed = cloudSpeed; });
        weatherT["cloudTexture"] = sol::property(
            [vfs](
                const MWWorld::Weather& w) { return Misc::ResourceHelpers::correctTexturePath(w.mCloudTexture, vfs); },
            [](MWWorld::Weather& w, std::string_view cloudTexture) { w.mCloudTexture = cloudTexture; });
        weatherT["cloudsMaximumPercent"]
            = sol::property([](const MWWorld::Weather& w) { return w.mCloudsMaximumPercent; },
                [](MWWorld::Weather& w, const FiniteFloat cloudsMaximumPercent) {
                    if (cloudsMaximumPercent <= 0.f)
                        throw std::runtime_error("Value must be greater than 0");
                    w.mCloudsMaximumPercent = cloudsMaximumPercent;
                });
        weatherT["isStorm"] = sol::property([](const MWWorld::Weather& w) { return w.mIsStorm; },
            [](MWWorld::Weather& w, bool isStorm) { w.mIsStorm = isStorm; });
        weatherT["stormDirection"] = sol::property([](const MWWorld::Weather& w) { return w.mStormDirection; },
            [](MWWorld::Weather& w, const FiniteVec3f& stormDirection) { w.mStormDirection = stormDirection; });
        weatherT["glareView"] = sol::property([](const MWWorld::Weather& w) { return w.mGlareView; },
            [](MWWorld::Weather& w, const FiniteFloat glareView) { w.mGlareView = glareView; });
        weatherT["rainSpeed"] = sol::property([](const MWWorld::Weather& w) { return w.mRainSpeed; },
            [](MWWorld::Weather& w, const FiniteFloat rainSpeed) { w.mRainSpeed = rainSpeed; });
        weatherT["rainEntranceSpeed"] = sol::property([](const MWWorld::Weather& w) { return w.mRainEntranceSpeed; },
            [](MWWorld::Weather& w, const FiniteFloat rainEntranceSpeed) {
                if (rainEntranceSpeed <= 0.f)
                    throw std::runtime_error("Value must be greater than 0");
                w.mRainEntranceSpeed = rainEntranceSpeed;
            });
        weatherT["rainEffect"] = sol::property(
            [](const MWWorld::Weather& w) -> sol::optional<std::string> {
                if (w.mRainEffect.empty())
                    return sol::nullopt;
                return w.mRainEffect;
            },
            [](MWWorld::Weather& w, sol::optional<std::string_view> rainEffect) {
                w.mRainEffect = rainEffect.value_or("");
            });
        weatherT["rainMaxRaindrops"] = sol::property([](const MWWorld::Weather& w) { return w.mRainMaxRaindrops; },
            [](MWWorld::Weather& w, int rainMaxRaindrops) { w.mRainMaxRaindrops = rainMaxRaindrops; });
        weatherT["rainDiameter"] = sol::property([](const MWWorld::Weather& w) { return w.mRainDiameter; },
            [](MWWorld::Weather& w, const FiniteFloat rainDiameter) { w.mRainDiameter = rainDiameter; });
        weatherT["rainThreshold"] = sol::property([](const MWWorld::Weather& w) { return w.mRainThreshold; },
            [](MWWorld::Weather& w, const FiniteFloat rainThreshold) { w.mRainThreshold = rainThreshold; });
        weatherT["rainMaxHeight"] = sol::property([](const MWWorld::Weather& w) { return w.mRainMaxHeight; },
            [](MWWorld::Weather& w, const FiniteFloat rainMaxHeight) { w.mRainMaxHeight = rainMaxHeight; });
        weatherT["rainMinHeight"] = sol::property([](const MWWorld::Weather& w) { return w.mRainMinHeight; },
            [](MWWorld::Weather& w, const FiniteFloat rainMinHeight) { w.mRainMinHeight = rainMinHeight; });
        weatherT["rainLoopSoundID"]
            = sol::property([](const MWWorld::Weather& w) { return LuaUtil::serializeRefId(w.mRainLoopSoundID); },
                [](MWWorld::Weather& w, sol::optional<std::string_view> rainLoopSoundID) {
                    w.mRainLoopSoundID = ESM::RefId::deserializeText(rainLoopSoundID.value_or(""));
                });
        weatherT["sunDiscSunsetColor"]
            = sol::property([](const MWWorld::Weather& w) { return Misc::Color::fromVec(w.mSunDiscSunsetColor); },
                [](MWWorld::Weather& w, const Misc::Color& sunDiscSunsetColor) {
                    w.mSunDiscSunsetColor = sunDiscSunsetColor.toVec();
                });
        weatherT["ambientLoopSoundID"]
            = sol::property([](const MWWorld::Weather& w) { return LuaUtil::serializeRefId(w.mAmbientLoopSoundID); },
                [](MWWorld::Weather& w, sol::optional<std::string_view> ambientLoopSoundId) {
                    w.mAmbientLoopSoundID = ESM::RefId::deserializeText(ambientLoopSoundId.value_or(""));
                });
        weatherT["ambientColor"] = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mAmbientColor; });
        weatherT["fogColor"] = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mFogColor; });
        weatherT["skyColor"] = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mSkyColor; });
        weatherT["sunColor"] = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mSunColor; });
        weatherT["landFogDepth"] = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mLandFogDepth; });
        weatherT["particleEffect"] = sol::property(
            [](const MWWorld::Weather& w) -> sol::optional<std::string> {
                if (w.mParticleEffect.empty())
                    return sol::nullopt;
                return w.mParticleEffect;
            },
            [](MWWorld::Weather& w, sol::optional<std::string_view> particleEffect) {
                w.mParticleEffect = particleEffect.value_or("");
            });
        weatherT["distantLandFogFactor"] = sol::property([](const MWWorld::Weather& w) { return w.mDL.FogFactor; },
            [](MWWorld::Weather& w, const FiniteFloat fogFactor) { w.mDL.FogFactor = fogFactor; });
        weatherT["distantLandFogOffset"] = sol::property([](const MWWorld::Weather& w) { return w.mDL.FogOffset; },
            [](MWWorld::Weather& w, const FiniteFloat fogOffset) { w.mDL.FogOffset = fogOffset; });

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

        api["getCurrent"] = overloadForActiveCell(
            []() -> const MWWorld::Weather* { return &MWBase::Environment::get().getWorld()->getCurrentWeather(); });
        api["getNext"] = overloadForActiveCell(
            []() -> const MWWorld::Weather* { return MWBase::Environment::get().getWorld()->getNextWeather(); });
        api["getTransition"] = overloadWeatherGetter(&MWBase::World::getWeatherTransition);
        api["getCurrentSunLightDirection"] = overloadForActiveCell(
            []() -> std::optional<osg::Vec4f> {
                osg::Vec4f sunPos = MWBase::Environment::get().getWorld()->getSunLightPosition();
                // normalize to get the direction towards the sun
                sunPos.normalize();

                // and invert it to get the direction of the sun light
                return -sunPos;
            },
            false);
        api["getCurrentSunVisibility"] = overloadWeatherGetter(&MWBase::World::getSunVisibility);
        api["getCurrentSunPercentage"] = overloadWeatherGetter(&MWBase::World::getSunPercentage);
        api["getCurrentWindSpeed"] = overloadWeatherGetter(&MWBase::World::getWindSpeed);
        api["getCurrentStormDirection"] = overloadWeatherGetter(&MWBase::World::getStormDirection);

        return LuaUtil::makeReadOnly(api);
    }
}
