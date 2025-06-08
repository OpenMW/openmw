#ifndef OPENMW_COMPONENTS_MISC_FINITENUMBERS_HPP
#define OPENMW_COMPONENTS_MISC_FINITENUMBERS_HPP

#include <components/lua/luastate.hpp>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace Misc
{
    template <class T>
    struct FiniteNumber
    {
        T mValue;

        FiniteNumber(T v)
        {
            if (!std::isfinite(v))
                throw std::invalid_argument("Value must be a finite number");
            mValue = v;
        }

        operator T() const { return mValue; }
    };

    using FiniteDouble = FiniteNumber<double>;

    using FiniteFloat = FiniteNumber<float>;
}

namespace sol
{
    template <class Handler, class T>
    bool sol_lua_check(
        types<Misc::FiniteNumber<T>>, lua_State* state, int index, Handler&& handler, stack::record& tracking)
    {
        bool success = stack::check<T>(state, lua_absindex(state, index), std::forward<Handler>(handler));
        tracking.use(1);
        return success;
    }

    template <class T>
    static Misc::FiniteNumber<T> sol_lua_get(
        types<Misc::FiniteNumber<T>>, lua_State* state, int index, stack::record& tracking)
    {
        T value = stack::get<T>(state, lua_absindex(state, index));
        tracking.use(1);
        return value;
    }
}

#endif
