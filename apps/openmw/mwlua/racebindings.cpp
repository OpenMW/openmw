#include "racebindings.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/lua/luastate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

#include "idcollectionbindings.hpp"
#include "types/types.hpp"

namespace
{
    struct RaceAttributes
    {
        const ESM::Race& mRace;
        const sol::state_view mLua;

        sol::table getAttribute(ESM::RefId id) const
        {
            sol::table res(mLua, sol::create);
            res["male"] = mRace.mData.getAttribute(id, true);
            res["female"] = mRace.mData.getAttribute(id, false);
            return LuaUtil::makeReadOnly(res);
        }
    };
}

namespace sol
{
    template <>
    struct is_automagical<ESM::Race> : std::false_type
    {
    };
    template <>
    struct is_automagical<RaceAttributes> : std::false_type
    {
    };
}

namespace MWLua
{
    sol::table initRaceRecordBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table races(lua, sol::create);
        addRecordFunctionBinding<ESM::Race>(races, context);

        auto raceT = lua.new_usertype<ESM::Race>("ESM3_Race");
        raceT[sol::meta_function::to_string]
            = [](const ESM::Race& rec) -> std::string { return "ESM3_Race[" + rec.mId.toDebugString() + "]"; };
        raceT["id"] = sol::readonly_property([](const ESM::Race& rec) { return rec.mId.serializeText(); });
        raceT["name"] = sol::readonly_property([](const ESM::Race& rec) -> std::string_view { return rec.mName; });
        raceT["description"]
            = sol::readonly_property([](const ESM::Race& rec) -> std::string_view { return rec.mDescription; });
        raceT["spells"] = sol::readonly_property(
            [lua](const ESM::Race& rec) -> sol::table { return createReadOnlyRefIdTable(lua, rec.mPowers.mList); });
        raceT["skills"] = sol::readonly_property([lua](const ESM::Race& rec) -> sol::table {
            sol::table res(lua, sol::create);
            for (const auto& skillBonus : rec.mData.mBonus)
            {
                ESM::RefId skill = ESM::Skill::indexToRefId(skillBonus.mSkill);
                if (!skill.empty())
                    res[skill.serializeText()] = skillBonus.mBonus;
            }
            return res;
        });
        raceT["isPlayable"] = sol::readonly_property(
            [](const ESM::Race& rec) -> bool { return rec.mData.mFlags & ESM::Race::Playable; });
        raceT["isBeast"]
            = sol::readonly_property([](const ESM::Race& rec) -> bool { return rec.mData.mFlags & ESM::Race::Beast; });
        raceT["height"] = sol::readonly_property([lua](const ESM::Race& rec) -> sol::table {
            sol::table res(lua, sol::create);
            res["male"] = rec.mData.mMaleHeight;
            res["female"] = rec.mData.mFemaleHeight;
            return LuaUtil::makeReadOnly(res);
        });
        raceT["weight"] = sol::readonly_property([lua](const ESM::Race& rec) -> sol::table {
            sol::table res(lua, sol::create);
            res["male"] = rec.mData.mMaleWeight;
            res["female"] = rec.mData.mFemaleWeight;
            return LuaUtil::makeReadOnly(res);
        });

        raceT["attributes"] = sol::readonly_property([lua](const ESM::Race& rec) -> RaceAttributes {
            return { rec, lua };
        });

        auto attributesT = lua.new_usertype<RaceAttributes>("ESM3_RaceAttributes");
        const auto& store = MWBase::Environment::get().getESMStore()->get<ESM::Attribute>();
        attributesT[sol::meta_function::index]
            = [&](const RaceAttributes& attributes, std::string_view stringId) -> sol::optional<sol::table> {
            ESM::RefId id = ESM::RefId::deserializeText(stringId);
            if (!store.search(id))
                return sol::nullopt;
            return attributes.getAttribute(id);
        };
        attributesT[sol::meta_function::pairs] = [&](sol::this_state ts, RaceAttributes& attributes) {
            auto iterator = store.begin();
            return sol::as_function(
                [iterator, attributes,
                    &store]() mutable -> std::pair<sol::optional<std::string>, sol::optional<sol::table>> {
                    if (iterator != store.end())
                    {
                        ESM::RefId id = iterator->mId;
                        ++iterator;
                        return { id.serializeText(), attributes.getAttribute(id) };
                    }
                    return { sol::nullopt, sol::nullopt };
                });
        };

        return LuaUtil::makeReadOnly(races);
    }
}
