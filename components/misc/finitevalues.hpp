#ifndef OPENMW_COMPONENTS_MISC_FINITEVALUES_HPP
#define OPENMW_COMPONENTS_MISC_FINITEVALUES_HPP

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/lua/luastate.hpp>

#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Misc
{
    template <class T>
    struct FiniteValue
    {
        T mValue;

        FiniteValue(T v)
        {
            constexpr bool isVecType
                = std::is_same_v<osg::Vec2f, T> || std::is_same_v<osg::Vec3f, T> || std::is_same_v<osg::Vec4f, T>;

            if constexpr (isVecType)
            {
                for (int i = 0; i < T::num_components; ++i)
                    if (!std::isfinite(v[i]))
                        throw std::invalid_argument("Vector must contain finite numbers");
            }
            else
            {
                if (!std::isfinite(v))
                    throw std::invalid_argument("Value must be a finite number");
            }
            mValue = v;
        }

        operator T() const { return mValue; }
    };

    using FiniteDouble = FiniteValue<double>;

    using FiniteFloat = FiniteValue<float>;

    using FiniteVec2f = FiniteValue<osg::Vec2f>;

    using FiniteVec3f = FiniteValue<osg::Vec3f>;

    using FiniteVec4f = FiniteValue<osg::Vec4f>;
}

namespace sol
{
    template <class Handler, class T>
    bool sol_lua_check(
        types<Misc::FiniteValue<T>>, lua_State* state, int index, Handler&& handler, stack::record& tracking)
    {
        bool success = stack::check<T>(state, lua_absindex(state, index), std::forward<Handler>(handler));
        tracking.use(1);
        return success;
    }

    template <class T>
    static Misc::FiniteValue<T> sol_lua_get(
        types<Misc::FiniteValue<T>>, lua_State* state, int index, stack::record& tracking)
    {
        T value = stack::get<T>(state, lua_absindex(state, index));
        tracking.use(1);
        return value;
    }
}

#endif
