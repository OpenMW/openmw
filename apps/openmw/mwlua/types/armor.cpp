#include "types.hpp"

#include <components/esm3/loadarmo.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Armor> : std::false_type
    {
    };
}

namespace MWLua
{
    void addArmorBindings(sol::table armor, const Context& context)
    {
        armor["TYPE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            { "Helmet", ESM::Armor::Helmet },
            { "Cuirass", ESM::Armor::Cuirass },
            { "LPauldron", ESM::Armor::LPauldron },
            { "RPauldron", ESM::Armor::RPauldron },
            { "Greaves", ESM::Armor::Greaves },
            { "Boots", ESM::Armor::Boots },
            { "LGauntlet", ESM::Armor::LGauntlet },
            { "RGauntlet", ESM::Armor::RGauntlet },
            { "Shield", ESM::Armor::Shield },
            { "LBracer", ESM::Armor::LBracer },
            { "RBracer", ESM::Armor::RBracer },
        }));

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        addRecordFunctionBinding<ESM::Armor>(armor, context);

        sol::usertype<ESM::Armor> record = context.mLua->sol().new_usertype<ESM::Armor>("ESM3_Armor");
        record[sol::meta_function::to_string]
            = [](const ESM::Armor& rec) -> std::string { return "ESM3_Armor[" + rec.mId.toDebugString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mId.serializeText(); });
        record["name"] = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Armor& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Armor& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["enchant"]
            = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mEnchant.serializeText(); });
        record["mwscript"]
            = sol::readonly_property([](const ESM::Armor& rec) -> std::string { return rec.mScript.serializeText(); });
        record["weight"] = sol::readonly_property([](const ESM::Armor& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mValue; });
        record["type"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mType; });
        record["health"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mHealth; });
        record["baseArmor"] = sol::readonly_property([](const ESM::Armor& rec) -> int { return rec.mData.mArmor; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Armor& rec) -> float { return rec.mData.mEnchant * 0.1f; });
    }
}
