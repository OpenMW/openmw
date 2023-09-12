#ifndef MWLUA_ITEMSTATS_H
#define MWLUA_ITEMSTATS_H

#include <sol/sol.hpp>

#include "../../mwworld/class.hpp"

#include "../objectvariant.hpp"
#include "types.hpp"

namespace MWLua
{
    class ItemStat
    {
    public:
        ItemStat(const sol::object& object);

        sol::optional<double> getCondition() const;

        void setCondition(float cond) const;

        /*
         * set,get, enchantmentCharge, soul? etc..
         */

        ObjectVariant mObject;
    };
}
#endif // MWLUA_ITEMSTATS_H
