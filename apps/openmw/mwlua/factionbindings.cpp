#include "factionbindings.hpp"
#include "recordstore.hpp"

#include <components/esm3/loadfact.hpp>
#include <components/lua/luastate.hpp>
#include <components/lua/util.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/store.hpp"

#include "idcollectionbindings.hpp"

namespace
{
    struct FactionRank : ESM::RankData
    {
        std::string mRankName;
        ESM::RefId mFactionId;
        size_t mRankIndex;

        FactionRank(const ESM::RefId& factionId, const ESM::RankData& data, std::string_view rankName, size_t rankIndex)
            : ESM::RankData(data)
            , mRankName(rankName)
            , mFactionId(factionId)
            , mRankIndex(rankIndex)
        {
        }
    };
}

namespace sol
{
    template <>
    struct is_automagical<ESM::Faction> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWWorld::Store<FactionRank>> : std::false_type
    {
    };
}

namespace MWLua
{
    sol::table initCoreFactionBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table factions(lua, sol::create);
        addRecordFunctionBinding<ESM::Faction>(factions, context);
        // Faction record
        auto factionT = lua.new_usertype<ESM::Faction>("ESM3_Faction");
        factionT[sol::meta_function::to_string]
            = [](const ESM::Faction& rec) -> std::string { return "ESM3_Faction[" + rec.mId.toDebugString() + "]"; };
        factionT["id"] = sol::readonly_property([](const ESM::Faction& rec) { return rec.mId.serializeText(); });
        factionT["name"]
            = sol::readonly_property([](const ESM::Faction& rec) -> std::string_view { return rec.mName; });
        factionT["hidden"]
            = sol::readonly_property([](const ESM::Faction& rec) -> bool { return rec.mData.mIsHidden; });
        factionT["ranks"] = sol::readonly_property([&lua](const ESM::Faction& rec) {
            sol::table res(lua, sol::create);
            for (size_t i = 0; i < rec.mRanks.size() && i < rec.mData.mRankData.size(); i++)
            {
                if (rec.mRanks[i].empty())
                    break;

                res.add(FactionRank(rec.mId, rec.mData.mRankData[i], rec.mRanks[i], i));
            }

            return res;
        });
        factionT["reactions"] = sol::readonly_property([&lua](const ESM::Faction& rec) {
            sol::table res(lua, sol::create);
            for (const auto& [factionId, reaction] : rec.mReactions)
                res[factionId.serializeText()] = reaction;

            const auto* overrides
                = MWBase::Environment::get().getDialogueManager()->getFactionReactionOverrides(rec.mId);

            if (overrides != nullptr)
            {
                for (const auto& [factionId, reaction] : *overrides)
                    res[factionId.serializeText()] = reaction;
            }

            return res;
        });
        factionT["attributes"] = sol::readonly_property([&lua](const ESM::Faction& rec) {
            return createReadOnlyRefIdTable(lua, rec.mData.mAttribute, ESM::Attribute::indexToRefId);
        });
        factionT["skills"] = sol::readonly_property([&lua](const ESM::Faction& rec) {
            return createReadOnlyRefIdTable(lua, rec.mData.mSkills, ESM::Skill::indexToRefId);
        });
        auto rankT = lua.new_usertype<FactionRank>("ESM3_FactionRank");
        rankT[sol::meta_function::to_string] = [](const FactionRank& rec) -> std::string {
            return "ESM3_FactionRank[" + rec.mFactionId.toDebugString() + ", "
                + std::to_string(LuaUtil::toLuaIndex(rec.mRankIndex)) + "]";
        };
        rankT["name"]
            = sol::readonly_property([](const FactionRank& rec) -> std::string_view { return rec.mRankName; });
        rankT["primarySkillValue"] = sol::readonly_property([](const FactionRank& rec) { return rec.mPrimarySkill; });
        rankT["favouredSkillValue"] = sol::readonly_property([](const FactionRank& rec) { return rec.mFavouredSkill; });
        rankT["factionReaction"] = sol::readonly_property([](const FactionRank& rec) { return rec.mFactReaction; });
        rankT["attributeValues"] = sol::readonly_property([&lua](const FactionRank& rec) {
            sol::table res(lua, sol::create);
            res.add(rec.mAttribute1);
            res.add(rec.mAttribute2);
            return res;
        });
        return LuaUtil::makeReadOnly(factions);
    }
}
