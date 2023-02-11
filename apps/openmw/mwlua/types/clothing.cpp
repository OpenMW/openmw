#include "types.hpp"

#include <components/esm3/loadclot.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwbase/world.hpp>
#include <apps/openmw/mwworld/esmstore.hpp>

namespace sol
{
    template <>
    struct is_automagical<ESM::Clothing> : std::false_type
    {
    };
}

namespace MWLua
{
    void addClothingBindings(sol::table clothing, const Context& context)
    {
        clothing["TYPE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            { "Amulet", ESM::Clothing::Amulet },
            { "Belt", ESM::Clothing::Belt },
            { "LGlove", ESM::Clothing::LGlove },
            { "Pants", ESM::Clothing::Pants },
            { "RGlove", ESM::Clothing::RGlove },
            { "Ring", ESM::Clothing::Ring },
            { "Robe", ESM::Clothing::Robe },
            { "Shirt", ESM::Clothing::Shirt },
            { "Shoes", ESM::Clothing::Shoes },
            { "Skirt", ESM::Clothing::Skirt },
        }));

        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        const MWWorld::Store<ESM::Clothing>* store
            = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Clothing>();
        clothing["record"] = sol::overload(
            [](const Object& obj) -> const ESM::Clothing* { return obj.ptr().get<ESM::Clothing>()->mBase; },
            [store](const std::string& recordId) -> const ESM::Clothing* {
                return store->find(ESM::RefId::stringRefId(recordId));
            });
        sol::usertype<ESM::Clothing> record = context.mLua->sol().new_usertype<ESM::Clothing>("ESM3_Clothing");
        record[sol::meta_function::to_string]
            = [](const ESM::Clothing& rec) -> std::string { return "ESM3_Clothing[" + rec.mId.getRefIdString() + "]"; };
        record["id"]
            = sol::readonly_property([](const ESM::Clothing& rec) -> std::string { return rec.mId.getRefIdString(); });
        record["name"] = sol::readonly_property([](const ESM::Clothing& rec) -> std::string { return rec.mName; });
        record["model"] = sol::readonly_property([vfs](const ESM::Clothing& rec) -> std::string {
            return Misc::ResourceHelpers::correctMeshPath(rec.mModel, vfs);
        });
        record["icon"] = sol::readonly_property([vfs](const ESM::Clothing& rec) -> std::string {
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        record["enchant"] = sol::readonly_property(
            [](const ESM::Clothing& rec) -> std::string { return rec.mEnchant.getRefIdString(); });
        record["mwscript"] = sol::readonly_property(
            [](const ESM::Clothing& rec) -> std::string { return rec.mScript.getRefIdString(); });
        record["weight"] = sol::readonly_property([](const ESM::Clothing& rec) -> float { return rec.mData.mWeight; });
        record["value"] = sol::readonly_property([](const ESM::Clothing& rec) -> int { return rec.mData.mValue; });
        record["type"] = sol::readonly_property([](const ESM::Clothing& rec) -> int { return rec.mData.mType; });
        record["enchantCapacity"]
            = sol::readonly_property([](const ESM::Clothing& rec) -> float { return rec.mData.mEnchant * 0.1f; });
    }
}
