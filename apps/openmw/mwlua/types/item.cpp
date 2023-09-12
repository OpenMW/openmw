#include "itemstats.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWLua::ItemStat> : std::false_type
    {
    };
}

namespace MWLua
{

    void addItemBindings(sol::table item, const Context& context)
    {
        sol::usertype<ItemStat> ItemStats = context.mLua->sol().new_usertype<ItemStat>("ItemStat");
        ItemStats[sol::meta_function::new_index] = [](const ItemStat& i, const sol::variadic_args args) {
            throw std::runtime_error("Unknown itemStat property '" + args.get<std::string>() + "'");
        };
        ItemStats["condition"] = sol::property([](const ItemStat& i) { return i.getCondition(); },
            [](const ItemStat& i, float cond) { i.setCondition(cond); });

        item["getEnchantmentCharge"]
            = [](const Object& object) { return object.ptr().getCellRef().getEnchantmentCharge(); };
        item["setEnchantmentCharge"]
            = [](const GObject& object, float charge) { object.ptr().getCellRef().setEnchantmentCharge(charge); };
        item["isRestocking"]
            = [](const Object& object) -> bool { return object.ptr().getRefData().getCount(false) < 0; };
        item["itemStats"] = [](const sol::object& object) { return ItemStat(object); };
    }
}
