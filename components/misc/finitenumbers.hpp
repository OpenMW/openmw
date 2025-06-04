#ifndef OPENMW_COMPONENTS_MISC_FINITENUMBERS_HPP
#define OPENMW_COMPONENTS_MISC_FINITENUMBERS_HPP

#include <components/lua/luastate.hpp>

#include <cmath>
#include <stdexcept>

namespace Misc
{
    struct FiniteDouble
    {
        double mValue;
        FiniteDouble(double v)
        {
            if (!std::isfinite(v))
                throw std::invalid_argument("Value must be a finite number");
            mValue = v;
        }
        operator double() const { return mValue; }
    };

    struct FiniteFloat
    {
        float mValue;
        FiniteFloat(float v)
        {
            if (!std::isfinite(v))
                throw std::invalid_argument("Value must be a finite number");
            mValue = v;
        }
        operator float() const { return mValue; }
    };
}

namespace sol
{
    using FiniteDouble = Misc::FiniteDouble;
    using FiniteFloat = Misc::FiniteFloat;

    template <typename Handler>
    bool sol_lua_check(
        sol::types<FiniteDouble>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
    {
        bool success = sol::stack::check<double>(L, lua_absindex(L, index), handler);
        tracking.use(1);
        return success;
    }

    static FiniteDouble sol_lua_get(sol::types<FiniteDouble>, lua_State* L, int index, sol::stack::record& tracking)
    {
        double val = sol::stack::get<double>(L, lua_absindex(L, index));
        tracking.use(1);
        return FiniteDouble(val);
    }

    template <typename Handler>
    bool sol_lua_check(
        sol::types<FiniteFloat>, lua_State* L, int index, Handler&& handler, sol::stack::record& tracking)
    {
        bool success = sol::stack::check<float>(L, lua_absindex(L, index), handler);
        tracking.use(1);
        return success;
    }

    static FiniteFloat sol_lua_get(sol::types<FiniteFloat>, lua_State* L, int index, sol::stack::record& tracking)
    {
        float val = sol::stack::get<float>(L, lua_absindex(L, index));
        tracking.use(1);
        return FiniteFloat(val);
    }
}

#endif
