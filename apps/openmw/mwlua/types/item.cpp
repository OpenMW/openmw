#include <sol/sol.hpp>

#include "../../mwworld/class.hpp"

#include "../itemdata.hpp"

#include "types.hpp"

namespace MWLua
{
    void addItemBindings(sol::table item, const Context& context)
    {
        item["getEnchantmentCharge"]
            = [](const Object& object) { return object.ptr().getCellRef().getEnchantmentCharge(); };
        item["setEnchantmentCharge"]
            = [](const GObject& object, float charge) { object.ptr().getCellRef().setEnchantmentCharge(charge); };
        item["isRestocking"]
            = [](const Object& object) -> bool { return object.ptr().getCellRef().getCount(false) < 0; };

        addItemDataBindings(item, context);
    }
}
