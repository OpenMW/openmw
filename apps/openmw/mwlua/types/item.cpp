#include <components/esm3/loadligh.hpp>
#include <sol/sol.hpp>

#include "../../mwmechanics/spellutil.hpp"
#include "../../mwworld/class.hpp"

#include "../itemdata.hpp"

#include "types.hpp"

namespace MWLua
{
    void addItemBindings(sol::table item, const Context& context)
    {
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

        item["isCarriable"] = [](const Object& object) -> bool {
            if (object.ptr().getClass().isItem(object.ptr()))
            {
                return true;
            }
            return object.ptr().mRef->getType() == ESM::REC_LIGH
                && (object.ptr().get<ESM::Light>()->mBase->mData.mFlags & ESM::Light::Carry) != 0;
        };

        addItemDataBindings(item, context);
    }
}
