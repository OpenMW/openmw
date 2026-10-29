#include "weatherbindings.hpp"

#include <format>
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
#include "recordstore.hpp"
#include "types/usertypeutil.hpp"

namespace
{
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

    template <class T>
    struct BaseTimeOfDayInterpolator
    {
        using Interpolator = MWWorld::TimeOfDayInterpolator<T>;
        using Getter = Interpolator(MWWorld::Weather::*);

        MWLua::MutableRecord<MWWorld::Weather> mWeather;
        Getter mGetter;

        const Interpolator& find() const { return mWeather.find().*mGetter; }
        Interpolator& find() { return mWeather.find().*mGetter; }
    };

    struct ThunderSoundIds
    {
        MWLua::MutableRecord<MWWorld::Weather> mWeather;

        const std::array<ESM::RefId, 4>& find() const { return mWeather.find().mThunderSoundID; }
        std::array<ESM::RefId, 4>& find() { return mWeather.find().mThunderSoundID; }
    };

    bool addPropertyFromTable(const sol::table& rec, std::string_view key, float& property)
    {
        auto value = rec[key].get<std::optional<Misc::FiniteFloat>>();
        if (value)
        {
            property = *value;
            return true;
        }
        return false;
    }

    void addPropertyFromTable(const sol::table& rec, std::string_view key, osg::Vec3f& property)
    {
        auto value = rec[key].get<std::optional<Misc::FiniteVec3f>>();
        if (value)
            property = *value;
    }

    bool addPropertyFromTable(const sol::table& rec, std::string_view key, osg::Vec4f& property)
    {
        auto value = rec[key].get<std::optional<Misc::Color>>();
        if (value)
        {
            property = value->toVec();
            return true;
        }
        return false;
    }

    void addPropertyFromTable(const sol::table& rec, std::string_view key, ESM::RefId& property)
    {
        auto value = rec[key].get<std::optional<std::string_view>>();
        if (value)
            property = ESM::RefId::deserializeText(*value);
    }

    void addPropertyFromTable(const sol::table& rec, std::string_view key, bool& property)
    {
        auto value = rec[key].get<std::optional<bool>>();
        if (value)
            property = *value;
    }

    template <class T>
    void setInterpolatorFromTable(MWWorld::TimeOfDayInterpolator<T>& interpolator, const sol::object value)
    {
        if (value == sol::nil)
            interpolator.blank();
        else if (value.is<BaseTimeOfDayInterpolator<T>>())
            interpolator = value.as<BaseTimeOfDayInterpolator<T>>().find();
        else
        {
            auto table = value.as<sol::lua_table>();
            T v;
            if (addPropertyFromTable(table, "sunrise", v))
                interpolator.setSunriseValue(v);
            if (addPropertyFromTable(table, "day", v))
                interpolator.setDayValue(v);
            if (addPropertyFromTable(table, "sunset", v))
                interpolator.setSunsetValue(v);
            if (addPropertyFromTable(table, "night", v))
                interpolator.setNightValue(v);
        }
    }

    void setThunderFromTable(MWWorld::Weather& weather, const sol::object value)
    {
        if (value == sol::nil)
            weather.mThunderSoundID.fill({});
        else if (value.is<ThunderSoundIds>())
            weather.mThunderSoundID = value.as<ThunderSoundIds>().find();
        else
        {
            auto table = value.as<sol::lua_table>();
            weather.mThunderSoundID.fill({});
            const size_t length = table.size();
            if (length > weather.mThunderSoundID.size())
                throw std::runtime_error("cannot specify more than 4 thunder sound ids");
            for (size_t i = 0; i < length; ++i)
            {
                const ESM::RefId id = ESM::RefId::deserializeText(table.get<std::string_view>(LuaUtil::toLuaIndex(i)));
                weather.mThunderSoundID[i] = id;
            }
        }
    }
}

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
    template <>
    struct is_automagical<MWWorld::WeatherStore> : std::false_type
    {
    };
    template <class T>
    struct is_automagical<BaseTimeOfDayInterpolator<T>> : std::false_type
    {
    };
    template <>
    struct is_automagical<ThunderSoundIds> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace Types
    {
        template <class T>
        struct RecordType<BaseTimeOfDayInterpolator<T>>
        {
            using Record = MWWorld::TimeOfDayInterpolator<T>;
            constexpr static bool isMutable = true;

            static const Record& asRecord(const BaseTimeOfDayInterpolator<T>& rec) { return rec.find(); }
            static Record& asRecord(BaseTimeOfDayInterpolator<T>& rec) { return rec.find(); }
        };
    }

    namespace
    {
        template <class T>
        void createFloatInterpolator(sol::state_view lua, std::string_view name)
        {
            using Misc::FiniteFloat;
            using RT = Types::RecordType<T>;

            auto interT = lua.new_usertype<T>(name);

            interT["sunrise"] = sol::property([](const T& inter) { return RT::asRecord(inter).getSunriseValue(); },
                [](T& inter, FiniteFloat value) { RT::asRecord(inter).setSunriseValue(value); });
            interT["sunset"] = sol::property([](const T& inter) { return RT::asRecord(inter).getSunsetValue(); },
                [](T& inter, FiniteFloat value) { RT::asRecord(inter).setSunsetValue(value); });
            interT["day"] = sol::property([](const T& inter) { return RT::asRecord(inter).getDayValue(); },
                [](T& inter, FiniteFloat value) { RT::asRecord(inter).setDayValue(value); });
            interT["night"] = sol::property([](const T& inter) { return RT::asRecord(inter).getNightValue(); },
                [](T& inter, FiniteFloat value) { RT::asRecord(inter).setNightValue(value); });
        }

        template <class T>
        void createColorInterpolator(sol::state_view lua, std::string_view name)
        {
            using Misc::Color;
            using RT = Types::RecordType<T>;

            auto interT = lua.new_usertype<T>(name);

            interT["sunrise"]
                = sol::property([](const T& inter) { return Color::fromVec(RT::asRecord(inter).getSunriseValue()); },
                    [](T& inter, const Color& value) { RT::asRecord(inter).setSunriseValue(value.toVec()); });
            interT["sunset"]
                = sol::property([](const T& inter) { return Color::fromVec(RT::asRecord(inter).getSunsetValue()); },
                    [](T& inter, const Color& value) { RT::asRecord(inter).setSunsetValue(value.toVec()); });
            interT["day"]
                = sol::property([](const T& inter) { return Color::fromVec(RT::asRecord(inter).getDayValue()); },
                    [](T& inter, const Color& value) { RT::asRecord(inter).setDayValue(value.toVec()); });
            interT["night"]
                = sol::property([](const T& inter) { return Color::fromVec(RT::asRecord(inter).getNightValue()); },
                    [](T& inter, const Color& value) { RT::asRecord(inter).setNightValue(value.toVec()); });
        }

        template <class T>
        void addUserType(sol::state_view& lua, std::string_view name)
        {
            using Misc::FiniteFloat;
            using Misc::FiniteVec3f;

            auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

            sol::usertype<T> record = lua.new_usertype<T>(name);
            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return std::format("Weather[{}]", rec.mId.toDebugString()); };
            record["recordId"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &MWWorld::Weather::mName);
            Types::addProperty(record, "scriptId", &MWWorld::Weather::mScriptId);

            using RT = Types::RecordType<T>;

            record["windSpeed"] = sol::property([](const T& w) { return RT::asRecord(w).mWindSpeed; },
                [](T& w, const FiniteFloat windSpeed) { RT::asRecord(w).mWindSpeed = windSpeed; });
            record["cloudSpeed"] = sol::property([](const T& w) { return RT::asRecord(w).mCloudSpeed; },
                [](T& w, const FiniteFloat cloudSpeed) { RT::asRecord(w).mCloudSpeed = cloudSpeed; });
            record["cloudTexture"]
                = sol::property([](const T& w) -> std::string_view { return RT::asRecord(w).mCloudTexture; },
                    [vfs](T& w, std::string_view cloudTexture) {
                        RT::asRecord(w).mCloudTexture
                            = Misc::ResourceHelpers::correctTexturePath(VFS::Path::toNormalized(cloudTexture), *vfs);
                    });
            record["cloudsMaximumPercent"]
                = sol::property([](const T& w) { return RT::asRecord(w).mCloudsMaximumPercent; },
                    [](T& w, const FiniteFloat cloudsMaximumPercent) {
                        if (cloudsMaximumPercent <= 0.f)
                            throw std::runtime_error("Value must be greater than 0");
                        RT::asRecord(w).mCloudsMaximumPercent = cloudsMaximumPercent;
                    });
            record["isStorm"] = sol::property([](const T& w) { return RT::asRecord(w).mIsStorm; },
                [](T& w, bool isStorm) { RT::asRecord(w).mIsStorm = isStorm; });
            record["stormDirection"] = sol::property([](const T& w) { return RT::asRecord(w).mStormDirection; },
                [](T& w, const FiniteVec3f& stormDirection) { RT::asRecord(w).mStormDirection = stormDirection; });
            record["glareView"] = sol::property([](const T& w) { return RT::asRecord(w).mGlareView; },
                [](T& w, const FiniteFloat glareView) { RT::asRecord(w).mGlareView = glareView; });
            record["rainSpeed"] = sol::property([](const T& w) { return RT::asRecord(w).mRainSpeed; },
                [](T& w, const FiniteFloat rainSpeed) { RT::asRecord(w).mRainSpeed = rainSpeed; });
            record["rainEntranceSpeed"] = sol::property([](const T& w) { return RT::asRecord(w).mRainEntranceSpeed; },
                [](T& w, const FiniteFloat rainEntranceSpeed) {
                    if (rainEntranceSpeed <= 0.f)
                        throw std::runtime_error("Value must be greater than 0");
                    RT::asRecord(w).mRainEntranceSpeed = rainEntranceSpeed;
                });
            record["rainEffect"] = sol::property(
                [](const T& w) -> std::optional<std::string_view> {
                    if (RT::asRecord(w).mRainEffect.empty())
                        return {};
                    return RT::asRecord(w).mRainEffect.view();
                },
                [](T& w, std::optional<std::string_view> rainEffect) {
                    RT::asRecord(w).mRainEffect = VFS::Path::Normalized(rainEffect.value_or(std::string_view{}));
                });
            record["rainMaxRaindrops"] = sol::property([](const T& w) { return RT::asRecord(w).mRainMaxRaindrops; },
                [](T& w, int rainMaxRaindrops) { RT::asRecord(w).mRainMaxRaindrops = rainMaxRaindrops; });
            record["rainDiameter"] = sol::property([](const T& w) { return RT::asRecord(w).mRainDiameter; },
                [](T& w, const FiniteFloat rainDiameter) { RT::asRecord(w).mRainDiameter = rainDiameter; });
            record["rainThreshold"] = sol::property([](const T& w) { return RT::asRecord(w).mRainThreshold; },
                [](T& w, const FiniteFloat rainThreshold) { RT::asRecord(w).mRainThreshold = rainThreshold; });
            record["rainMaxHeight"] = sol::property([](const T& w) { return RT::asRecord(w).mRainMaxHeight; },
                [](T& w, const FiniteFloat rainMaxHeight) { RT::asRecord(w).mRainMaxHeight = rainMaxHeight; });
            record["rainMinHeight"] = sol::property([](const T& w) { return RT::asRecord(w).mRainMinHeight; },
                [](T& w, const FiniteFloat rainMinHeight) { RT::asRecord(w).mRainMinHeight = rainMinHeight; });
            record["rainLoopSoundID"]
                = sol::property([](const T& w) -> ESM::RefId { return RT::asRecord(w).mRainLoopSoundID; },
                    [](T& w, sol::optional<std::string_view> rainLoopSoundID) {
                        RT::asRecord(w).mRainLoopSoundID
                            = ESM::RefId::deserializeText(rainLoopSoundID.value_or(std::string_view{}));
                    });
            record["sunDiscSunsetColor"]
                = sol::property([](const T& w) { return Misc::Color::fromVec(RT::asRecord(w).mSunDiscSunsetColor); },
                    [](T& w, const Misc::Color& sunDiscSunsetColor) {
                        RT::asRecord(w).mSunDiscSunsetColor = sunDiscSunsetColor.toVec();
                    });
            record["ambientLoopSoundID"]
                = sol::property([](const T& w) -> ESM::RefId { return RT::asRecord(w).mAmbientLoopSoundID; },
                    [](T& w, sol::optional<std::string_view> ambientLoopSoundId) {
                        RT::asRecord(w).mAmbientLoopSoundID
                            = ESM::RefId::deserializeText(ambientLoopSoundId.value_or(std::string_view{}));
                    });
            record["particleEffect"] = sol::property(
                [](const T& w) -> std::optional<std::string_view> {
                    if (RT::asRecord(w).mParticleEffect.empty())
                        return {};
                    return RT::asRecord(w).mParticleEffect.view();
                },
                [](T& w, std::optional<std::string_view> particleEffect) {
                    RT::asRecord(w).mParticleEffect
                        = VFS::Path::Normalized(particleEffect.value_or(std::string_view{}));
                });
            record["distantLandFogFactor"] = sol::property([](const T& w) { return RT::asRecord(w).mDL.FogFactor; },
                [](T& w, const FiniteFloat fogFactor) { RT::asRecord(w).mDL.FogFactor = fogFactor; });
            record["distantLandFogOffset"] = sol::property([](const T& w) { return RT::asRecord(w).mDL.FogOffset; },
                [](T& w, const FiniteFloat fogOffset) { RT::asRecord(w).mDL.FogOffset = fogOffset; });

            record["transitionDelta"] = sol::property([](const T& w) { return RT::asRecord(w).mTransitionDelta; },
                [](T& w, const FiniteFloat transitionDelta) { RT::asRecord(w).mTransitionDelta = transitionDelta; });
            record["thunderFrequency"] = sol::property([](const T& w) { return RT::asRecord(w).mThunderFrequency; },
                [](T& w, const FiniteFloat thunderFrequency) { RT::asRecord(w).mThunderFrequency = thunderFrequency; });
            record["thunderThreshold"] = sol::property([](const T& w) { return RT::asRecord(w).mThunderThreshold; },
                [](T& w, const FiniteFloat thunderThreshold) { RT::asRecord(w).mThunderThreshold = thunderThreshold; });
            record["flashDecrement"] = sol::property([](const T& w) { return RT::asRecord(w).mFlashDecrement; },
                [](T& w, const FiniteFloat flashDecrement) { RT::asRecord(w).mFlashDecrement = flashDecrement; });
            record["stormParticleRotation"]
                = sol::property([](const T& w) { return RT::asRecord(w).mStormParticleRotation; },
                    [](T& w, const FiniteVec3f& stormParticleRotation) {
                        RT::asRecord(w).mStormParticleRotation = stormParticleRotation;
                    });
            record["hasRipples"] = sol::property([](const T& w) { return RT::asRecord(w).mRipplesEnabled; },
                [](T& w, bool ripplesEnabled) { RT::asRecord(w).mRipplesEnabled = ripplesEnabled; });
            record["canUseTorches"] = sol::property([](const T& w) { return RT::asRecord(w).mUseTorches; },
                [](T& w, bool useTorches) { RT::asRecord(w).mUseTorches = useTorches; });
            record["isOccluded"] = sol::property([](const T& w) { return RT::asRecord(w).mOcclusionEnabled; },
                [](T& w, bool occlusionEnabled) { RT::asRecord(w).mOcclusionEnabled = occlusionEnabled; });

            if constexpr (RT::isMutable)
            {
                record["thunderSoundID"] = sol::property([](const T& rec) { return ThunderSoundIds{ rec }; },
                    [](T& rec, const sol::object& value) {
                        MWWorld::Weather& weather = rec.find();
                        setThunderFromTable(weather, value);
                    });
                auto thunderT = lua.new_usertype<ThunderSoundIds>("BaseWeatherThunderSoundIds");
                thunderT[sol::meta_function::length] = [](const ThunderSoundIds& ids) { return ids.find().size(); };
                thunderT[sol::meta_function::index]
                    = [](const ThunderSoundIds& ids, uint32_t index) -> std::optional<ESM::RefId> {
                    const auto& array = ids.find();
                    if (index == 0 || index > array.size())
                        return {};
                    return array[index - 1];
                };
                thunderT[sol::meta_function::new_index]
                    = [](ThunderSoundIds& ids, uint32_t i, const sol::object& value) {
                          auto& array = ids.find();
                          if (i == 0 || i > array.size())
                              throw std::runtime_error("index out of range");
                          --i;
                          ESM::RefId id;
                          if (value != sol::nil)
                              id = ESM::RefId::deserializeText(value.as<std::string_view>());
                          array[i] = id;
                      };
                thunderT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
                thunderT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

                record["ambientColor"] = sol::property(
                    [](const T& w) {
                        return BaseTimeOfDayInterpolator<osg::Vec4f>{ w, &MWWorld::Weather::mAmbientColor };
                    },
                    [](T& rec, const sol::object& value) {
                        MWWorld::Weather& weather = rec.find();
                        setInterpolatorFromTable(weather.mAmbientColor, value);
                    });
                record["fogColor"] = sol::property(
                    [](const T& w) {
                        return BaseTimeOfDayInterpolator<osg::Vec4f>{ w, &MWWorld::Weather::mFogColor };
                    },
                    [](T& rec, const sol::object& value) {
                        MWWorld::Weather& weather = rec.find();
                        setInterpolatorFromTable(weather.mFogColor, value);
                    });
                record["skyColor"] = sol::property(
                    [](const T& w) {
                        return BaseTimeOfDayInterpolator<osg::Vec4f>{ w, &MWWorld::Weather::mSkyColor };
                    },
                    [](T& rec, const sol::object& value) {
                        MWWorld::Weather& weather = rec.find();
                        setInterpolatorFromTable(weather.mSkyColor, value);
                    });
                record["sunColor"] = sol::property(
                    [](const T& w) {
                        return BaseTimeOfDayInterpolator<osg::Vec4f>{ w, &MWWorld::Weather::mSunColor };
                    },
                    [](T& rec, const sol::object& value) {
                        MWWorld::Weather& weather = rec.find();
                        setInterpolatorFromTable(weather.mSunColor, value);
                    });
                record["landFogDepth"] = sol::property(
                    [](const T& w) {
                        return BaseTimeOfDayInterpolator<float>{ w, &MWWorld::Weather::mLandFogDepth };
                    },
                    [](T& rec, const sol::object& value) {
                        MWWorld::Weather& weather = rec.find();
                        setInterpolatorFromTable(weather.mLandFogDepth, value);
                    });
            }
            else
            {
                record["thunderSoundID"] = sol::readonly_property([lua](const MWWorld::Weather& w) {
                    sol::table result(lua, sol::create);
                    for (const auto& soundId : w.mThunderSoundID)
                        result.add(soundId.serializeText());
                    return result;
                });
                record["ambientColor"]
                    = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mAmbientColor; });
                record["fogColor"] = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mFogColor; });
                record["skyColor"] = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mSkyColor; });
                record["sunColor"] = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mSunColor; });
                record["landFogDepth"]
                    = sol::readonly_property([](const MWWorld::Weather& w) { return &w.mLandFogDepth; });
            }
        }
    }

    sol::table initCoreWeatherBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        addUserType<MWWorld::Weather>(lua, "Weather");
        createFloatInterpolator<MWWorld::TimeOfDayInterpolator<float>>(lua, "TimeOfDayInterpolatorFloat");
        createColorInterpolator<MWWorld::TimeOfDayInterpolator<osg::Vec4f>>(lua, "TimeOfDayInterpolatorColor");

        api["changeWeather"] = [](std::string_view regionId, const MWWorld::Weather& weather) {
            ESM::RefId region = ESM::RefId::deserializeText(regionId);
            MWBase::Environment::get().getESMStore()->get<ESM::Region>().find(region);
            MWBase::Environment::get().getWorld()->changeWeather(region, weather.mId);
        };

        addRecordStoreType<MWWorld::WeatherStore, MWWorld::Weather>(lua, "Weather");

        // Provide access to the store.
        api["records"] = &MWBase::Environment::get().getWorld()->getAllWeather();

        using Phase = MWRender::MoonState::Phase;
        api["MOON_PHASE"] = LuaUtil::makeStrictReadOnly(LuaUtil::tableFromPairs<std::string_view, Phase>(lua,
            { { "Full", Phase::Full }, { "WaningGibbous", Phase::WaningGibbous },
                { "ThirdQuarter", Phase::ThirdQuarter }, { "WaningCrescent", Phase::WaningCrescent },
                { "New", Phase::New }, { "WaxingCrescent", Phase::WaxingCrescent },
                { "FirstQuarter", Phase::FirstQuarter }, { "WaxingGibbous", Phase::WaxingGibbous } }));

        api["getCurrent"] = overloadForActiveCell(
            []() -> const MWWorld::Weather* { return MWBase::Environment::get().getWorld()->getCurrentWeather(); });
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
        api["getCurrentMoons"] = overloadForActiveCell([lua]() {
            sol::table result(lua, sol::create);
            for (const MWWorld::Moon& moon : MWBase::Environment::get().getWorld()->getCurrentMoons())
            {
                sol::table moonTable(lua, sol::create);
                moonTable["name"] = moon.mName;
                moonTable["phase"] = moon.mPhase;
                moonTable["phaseValue"] = moon.mPhaseValue;
                moonTable["alpha"] = moon.mAlpha;
                result.add(LuaUtil::makeReadOnly(moonTable));
            }
            return LuaUtil::makeReadOnly(result);
        });

        return LuaUtil::makeReadOnly(api);
    }

    MWWorld::Weather tableToWeather(const sol::table& rec)
    {
        auto weather = Types::initFromTemplate<MWWorld::Weather>(rec);
        if (rec["name"] != sol::nil)
            weather.mName = rec["name"];
        if (rec["scriptId"] != sol::nil)
            weather.mScriptId = rec["scriptId"];
        addPropertyFromTable(rec, "windSpeed", weather.mWindSpeed);
        addPropertyFromTable(rec, "cloudSpeed", weather.mCloudSpeed);
        if (auto cloudTexture = rec.get<std::optional<std::string_view>>("cloudTexture"))
        {
            auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
            weather.mCloudTexture
                = Misc::ResourceHelpers::correctTexturePath(VFS::Path::toNormalized(*cloudTexture), *vfs);
        }
        addPropertyFromTable(rec, "cloudsMaximumPercent", weather.mCloudsMaximumPercent);
        if (weather.mCloudsMaximumPercent <= 0.f)
            throw std::runtime_error("cloudsMaximumPercent must be greater than 0");
        addPropertyFromTable(rec, "isStorm", weather.mIsStorm);
        addPropertyFromTable(rec, "stormDirection", weather.mStormDirection);
        addPropertyFromTable(rec, "glareView", weather.mGlareView);
        addPropertyFromTable(rec, "rainSpeed", weather.mRainSpeed);
        addPropertyFromTable(rec, "rainEntranceSpeed", weather.mRainEntranceSpeed);
        if (weather.mRainEntranceSpeed <= 0.f)
            throw std::runtime_error("rainEntranceSpeed must be greater than 0");
        if (auto rainEffect = rec.get<std::optional<std::string_view>>("rainEffect"))
            weather.mRainEffect = VFS::Path::toNormalized(*rainEffect);
        if (rec["rainMaxRaindrops"] != sol::nil)
            weather.mRainMaxRaindrops = rec["rainMaxRaindrops"];
        addPropertyFromTable(rec, "rainDiameter", weather.mRainDiameter);
        addPropertyFromTable(rec, "rainThreshold", weather.mRainThreshold);
        addPropertyFromTable(rec, "rainMaxHeight", weather.mRainMaxHeight);
        addPropertyFromTable(rec, "rainMinHeight", weather.mRainMinHeight);
        addPropertyFromTable(rec, "rainLoopSoundID", weather.mRainLoopSoundID);
        if (rec["sunDiscSunsetColor"] != sol::nil)
            weather.mSunDiscSunsetColor = rec.get<Misc::Color>("sunDiscSunsetColor").toVec();
        addPropertyFromTable(rec, "ambientLoopSoundID", weather.mAmbientLoopSoundID);
        if (auto particleEffect = rec.get<std::optional<std::string_view>>("particleEffect"))
            weather.mParticleEffect = VFS::Path::toNormalized(*particleEffect);
        addPropertyFromTable(rec, "distantLandFogFactor", weather.mDL.FogFactor);
        addPropertyFromTable(rec, "distantLandFogOffset", weather.mDL.FogOffset);
        if (rec["thunderSoundID"] != sol::nil)
            setThunderFromTable(weather, rec["thunderSoundID"]);
        if (rec["ambientColor"] != sol::nil)
            setInterpolatorFromTable(weather.mAmbientColor, rec["ambientColor"]);
        if (rec["fogColor"] != sol::nil)
            setInterpolatorFromTable(weather.mFogColor, rec["fogColor"]);
        if (rec["skyColor"] != sol::nil)
            setInterpolatorFromTable(weather.mSkyColor, rec["skyColor"]);
        if (rec["sunColor"] != sol::nil)
            setInterpolatorFromTable(weather.mSunColor, rec["sunColor"]);
        if (rec["landFogDepth"] != sol::nil)
            setInterpolatorFromTable(weather.mLandFogDepth, rec["landFogDepth"]);
        addPropertyFromTable(rec, "transitionDelta", weather.mTransitionDelta);
        addPropertyFromTable(rec, "thunderFrequency", weather.mThunderFrequency);
        addPropertyFromTable(rec, "thunderThreshold", weather.mThunderThreshold);
        addPropertyFromTable(rec, "flashDecrement", weather.mFlashDecrement);
        addPropertyFromTable(rec, "stormParticleRotation", weather.mStormParticleRotation);
        addPropertyFromTable(rec, "hasRipples", weather.mRipplesEnabled);
        addPropertyFromTable(rec, "canUseTorches", weather.mUseTorches);
        addPropertyFromTable(rec, "isOccluded", weather.mOcclusionEnabled);
        return weather;
    }

    void addMutableWeatherType(sol::state_view& lua)
    {
        addUserType<MutableRecord<MWWorld::Weather>>(lua, "BaseWeather");
        createFloatInterpolator<BaseTimeOfDayInterpolator<float>>(lua, "BaseTimeOfDayInterpolatorFloat");
        createColorInterpolator<BaseTimeOfDayInterpolator<osg::Vec4f>>(lua, "BaseTimeOfDayInterpolatorColor");
    }

    const MWWorld::Weather& MutableRecord<MWWorld::Weather>::find() const
    {
        return *mStore.find(mId);
    }

    MWWorld::Weather& MutableRecord<MWWorld::Weather>::find()
    {
        return *const_cast<MWWorld::Weather*>(mStore.find(mId));
    }
}
