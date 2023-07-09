
#include "types.hpp"

namespace MWLua
{
    void addItemBindings(sol::table item)
    {
        item["getEnchantmentCharge"]
            = [](const Object& object) { return object.ptr().getCellRef().getEnchantmentCharge(); };
        item["setEnchantmentCharge"]
            = [](const GObject& object, float charge) { object.ptr().getCellRef().setEnchantmentCharge(charge); };
    }
}
