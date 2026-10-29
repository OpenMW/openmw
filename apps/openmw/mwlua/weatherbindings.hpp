#ifndef MWLUA_WEATHERBINDINGS_H
#define MWLUA_WEATHERBINDINGS_H

#include <sol/forward.hpp>

#include "contentbindings.hpp"

namespace MWWorld
{
    class Weather;
    class WeatherStore;
}

namespace MWLua
{
    template <>
    struct MutableRecord<MWWorld::Weather>
    {
        MWWorld::WeatherStore& mStore;
        ESM::RefId mId;

        const MWWorld::Weather& find() const;
        MWWorld::Weather& find();
    };

    struct Context;

    sol::table initCoreWeatherBindings(const Context&);

    MWWorld::Weather tableToWeather(const sol::table&);

    void addMutableWeatherType(sol::state_view& lua);
}

#endif // MWLUA_WEATHERBINDINGS_H
