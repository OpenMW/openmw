#include <sol/sol.hpp>

#include "../../mwmechanics/spellutil.hpp"
#include "../../mwworld/class.hpp"

#include "../itemdata.hpp"

#include "types.hpp"

namespace MWLua
{
    void addItemBindings(sol::table item, const Context& context)
    {
        // Deprecated. Moved to itemData; should be removed later
        item["getEnchantmentCharge"] = [](const Object& object) -> sol::optional<float> {
            float charge = object.ptr().getCellRef().getEnchantmentCharge();
            if (charge == -1)
                return sol::nullopt;
            else
                return charge;
        };
        item["setEnchantmentCharge"] = [](const GObject& object, sol::optional<float> charge) {
            object.ptr().getCellRef().setEnchantmentCharge(charge.value_or(-1));
        };
        item["isRestocking"]
            = [](const Object& object) -> bool { return object.ptr().getCellRef().getCount(false) < 0; };

        item["isCarriable"] = [](const Object& object) -> bool { return object.ptr().getClass().isItem(object.ptr()); };

        addItemDataBindings(item, context);
    }
}
