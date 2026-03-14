#include "types.hpp"

#include "../magictypebindings.hpp"
#include "usertypeutil.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>
#include <components/misc/finitevalues.hpp>
#include <components/misc/resourcehelpers.hpp>

namespace MWLua
{
    // Populates a potion struct from a Lua table.
    ESM::Potion tableToPotion(const sol::table& rec)
    {
        auto potion = Types::initFromTemplate<ESM::Potion>(rec);
        if (rec["name"] != sol::nil)
            potion.mName = rec["name"];
        if (rec["model"] != sol::nil)
            potion.mModel = Misc::ResourceHelpers::meshPathForESM3(rec["model"].get<std::string_view>());
        if (rec["icon"] != sol::nil)
            potion.mIcon = rec["icon"];
        if (rec["mwscript"] != sol::nil)
        {
            std::string_view scriptId = rec["mwscript"].get<std::string_view>();
            potion.mScript = ESM::RefId::deserializeText(scriptId);
        }
        if (rec["weight"] != sol::nil)
            potion.mData.mWeight = rec["weight"].get<Misc::FiniteFloat>();
        if (rec["value"] != sol::nil)
            potion.mData.mValue = rec["value"];
        if (rec["effects"] != sol::nil)
        {
            sol::table effectsTable = rec["effects"];
            size_t numEffects = effectsTable.size();
            potion.mEffects.mList.resize(numEffects);
            for (size_t i = 0; i < numEffects; ++i)
            {
                potion.mEffects.mList[i] = LuaUtil::cast<ESM::IndexedENAMstruct>(effectsTable[LuaUtil::toLuaIndex(i)]);
            }
            potion.mEffects.updateIndexes();
        }
        if (rec["isAutocalc"] != sol::nil)
            potion.mData.mFlags = rec["isAutocalc"] ? ESM::Potion::Autocalc : 0;

        return potion;
    }

    void addPotionBindings(sol::table potion, const Context& context)
    {
        addRecordFunctionBinding<ESM::Potion>(potion, context);

        // Creates a new potion struct but does not store it in MWWorld::ESMStore.
        // Global scripts can use world.createRecord to add the potion to the world.
        // Note: This potion instance must be owned by lua, so we return it
        // by value.
        potion["createRecordDraft"] = tableToPotion;

        sol::state_view lua = context.sol();
        addPotionType(lua);
    }
}