#include "birthsignbindings.hpp"

#include <components/esm3/loadbsgn.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"

#include "idcollectionbindings.hpp"
#include "recordstore.hpp"

namespace sol
{
    template <>
    struct is_automagical<ESM::BirthSign> : std::false_type
    {
    };
}

namespace MWLua
{
    sol::table initBirthSignRecordBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table birthSigns(lua, sol::create);
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
            return Misc::ResourceHelpers::correctTexturePath(VFS::Path::toNormalized(rec.mTexture), *vfs);
        });
        signT["spells"] = sol::readonly_property([lua](const ESM::BirthSign& rec) -> sol::table {
            return createReadOnlyRefIdTable(lua, rec.mPowers.mList);
        });

        return LuaUtil::makeReadOnly(birthSigns);
    }
}
