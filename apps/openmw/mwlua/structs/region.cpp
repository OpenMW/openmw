#include "region.hpp"

#include "../luamanager.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include <components/esm/loadregn.hpp>
#include <components/esm/weatherstate.hpp>

namespace MWLua
{
    void bindTES3Region()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Binding for ESM::Region::SoundRef
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::Region::SoundRef>();
            usertypeDefinition.set("new", sol::no_constructor);

            usertypeDefinition.set("sound", sol::property(
                [](ESM::Region::SoundRef& self) { return self.mSound; },
                [](ESM::Region::SoundRef& self, const char* value) { if (strlen(value) < 32) self.mSound.assign(value); }
            ));

            // Restrict chance [0-100].
            usertypeDefinition.set("chance", sol::property(
                [](ESM::Region::SoundRef& self) { return self.mChance; },
                [](ESM::Region::SoundRef& self, int value)
                {
                    if (value < 0)
                    {
                        value = 0;
                    }
                    else if (value > 100)
                    {
                        value = 100;
                    }

                    self.mChance = value;
                }
            ));

            // Finish up our usertype.
            state.set_usertype("tes3regionSound", usertypeDefinition);
        }

        // Binding for ESM::Region.
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::Region>();
            usertypeDefinition.set("new", sol::no_constructor);

            usertypeDefinition.set("id", &ESM::Region::mId);

            // Basic property binding.
            usertypeDefinition.set("name", &ESM::Region::mName);
            usertypeDefinition.set("sleepCreature", &ESM::Region::mSleepList);
            usertypeDefinition.set("sounds", sol::property(&ESM::Region::mSoundList));

            // User-friendly access to weather chances.
            usertypeDefinition.set("weatherChanceAsh", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Ash); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Ash, value); }
            ));
            usertypeDefinition.set("weatherChanceBlight", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Blight); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Blight, value); }
            ));
            usertypeDefinition.set("weatherChanceBlizzard", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Blizzard); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Blizzard, value); }
            ));
            usertypeDefinition.set("weatherChanceClear", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Clear); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Clear, value); }
            ));
            usertypeDefinition.set("weatherChanceCloudy", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Cloudy); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Cloudy, value); }
            ));
            usertypeDefinition.set("weatherChanceFoggy", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Foggy); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Foggy, value); }
            ));
            usertypeDefinition.set("weatherChanceOvercast", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Overcast); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Overcast, value); }
            ));
            usertypeDefinition.set("weatherChanceRain", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Rain); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Rain, value); }
            ));
            usertypeDefinition.set("weatherChanceSnow", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Snow); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Snow, value); }
            ));
            usertypeDefinition.set("weatherChanceThunder", sol::property(
                [](ESM::Region& self) { return MWBase::Environment::get().getWorld()->getWeatherChance(self.mId, ESM::WeatherType::Thunder); },
                [](ESM::Region& self, unsigned char value) { MWBase::Environment::get().getWorld()->modRegion(self.mId, ESM::WeatherType::Thunder, value); }
            ));

            /*
            // Expose the current weather, and allow it to be changed via setting.
            usertypeDefinition.set("weather", sol::property(
                [](TES3::Region& self) -> sol::object
            {
                if (self.currentWeatherIndex < TES3::WeatherType::First || self.currentWeatherIndex > TES3::WeatherType::Last) {
                    return sol::nil;
                }
                return makeLuaObject(TES3::WorldController::get()->weatherController->arrayWeathers[self.currentWeatherIndex]);
            },
                [](TES3::Region& self, sol::object weather)
            {
                // Get the index, either from a weather object or directly as a number.
                int index = -1;
                if (weather.is<TES3::Weather>()) {
                    index = weather.as<TES3::Weather*>()->index;
                }
                else if (weather.is<int>()) {
                    index = weather.as<int>();
                }

                // If it was a valid and different index, change the weather to it.
                if (index != self.currentWeatherIndex && index >= TES3::WeatherType::First && index <= TES3::WeatherType::Last) {
                    self.changeWeather(index);
                }
            }
            ));

            // Basic function binding.
            usertypeDefinition.set("changeWeather", &TES3::Region::changeWeather);
            usertypeDefinition.set("randomizeWeather", &TES3::Region::randomizeWeather);
            */

            // Finish up our usertype.
            state.set_usertype("tes3region", usertypeDefinition);
        }
    }
}
