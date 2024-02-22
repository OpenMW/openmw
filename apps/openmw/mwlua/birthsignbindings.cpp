#include <components/esm3/loadbsgn.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "birthsignbindings.hpp"
#include "luamanagerimp.hpp"
#include "types/types.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::BirthSign> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWWorld::Store<ESM::BirthSign>> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::SpellList> : std::false_type
    {
    };
}

namespace MWLua
{
    sol::table initBirthSignRecordBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::table birthSigns(context.mLua->sol(), sol::create);
        addRecordFunctionBinding<ESM::BirthSign>(birthSigns, context);

        auto signT = lua.new_usertype<ESM::BirthSign>("ESM3_BirthSign");
        signT[sol::meta_function::to_string] = [](const ESM::BirthSign& rec) -> std::string {
            return "ESM3_BirthSign[" + rec.mId.toDebugString() + "]";
        };
        signT["id"] = sol::readonly_property([](const ESM::BirthSign& rec) { return rec.mId.serializeText(); });
        signT["name"] = sol::readonly_property([](const ESM::BirthSign& rec) -> std::string_view { return rec.mName; });
        signT["description"]
            = sol::readonly_property([](const ESM::BirthSign& rec) -> std::string_view { return rec.mDescription; });
        auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        signT["texture"] = sol::readonly_property([vfs](const ESM::BirthSign& rec) -> std::string {
            return Misc::ResourceHelpers::correctTexturePath(rec.mTexture, vfs);
        });
        signT["spells"]
            = sol::readonly_property([](const ESM::BirthSign& rec) -> const ESM::SpellList* { return &rec.mPowers; });

        auto spellListT = lua.new_usertype<ESM::SpellList>("ESM3_SpellList");
        spellListT[sol::meta_function::length] = [](const ESM::SpellList& list) { return list.mList.size(); };
        spellListT[sol::meta_function::index]
            = [](const ESM::SpellList& list, size_t index) -> sol::optional<std::string> {
            if (index == 0 || index > list.mList.size())
                return sol::nullopt;
            return list.mList[index - 1].serializeText(); // Translate from Lua's 1-based indexing.
        };
        spellListT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        spellListT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();

        return LuaUtil::makeReadOnly(birthSigns);
    }
}
