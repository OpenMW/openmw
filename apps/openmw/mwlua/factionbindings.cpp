#include "factionbindings.hpp"

#include <components/esm3/loadfact.hpp>
#include <components/lua/luastate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "luamanagerimp.hpp"

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
    struct is_automagical<MWWorld::Store<ESM::Faction>> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWWorld::Store<FactionRank>> : std::false_type
    {
    };
}

namespace MWLua
{
    using FactionStore = MWWorld::Store<ESM::Faction>;

    void initCoreFactionBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::usertype<FactionStore> factionStoreT = lua.new_usertype<FactionStore>("ESM3_FactionStore");
        factionStoreT[sol::meta_function::to_string] = [](const FactionStore& store) {
            return "ESM3_FactionStore{" + std::to_string(store.getSize()) + " factions}";
        };
        factionStoreT[sol::meta_function::length] = [](const FactionStore& store) { return store.getSize(); };
        factionStoreT[sol::meta_function::index] = sol::overload(
            [](const FactionStore& store, size_t index) -> const ESM::Faction* {
                if (index == 0 || index > store.getSize())
                    return nullptr;
                return store.at(index - 1);
            },
            [](const FactionStore& store, std::string_view factionId) -> const ESM::Faction* {
                return store.search(ESM::RefId::deserializeText(factionId));
            });
        factionStoreT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        factionStoreT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
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
            return res;
        });
        factionT["attributes"] = sol::readonly_property([&lua](const ESM::Faction& rec) {
            sol::table res(lua, sol::create);
            for (auto attributeIndex : rec.mData.mAttribute)
            {
                ESM::RefId id = ESM::Attribute::indexToRefId(attributeIndex);
                if (!id.empty())
                    res.add(id.serializeText());
            }

            return res;
        });
        factionT["skills"] = sol::readonly_property([&lua](const ESM::Faction& rec) {
            sol::table res(lua, sol::create);
            for (auto skillIndex : rec.mData.mSkills)
            {
                ESM::RefId id = ESM::Skill::indexToRefId(skillIndex);
                if (!id.empty())
                    res.add(id.serializeText());
            }

            return res;
        });
        auto rankT = lua.new_usertype<FactionRank>("ESM3_FactionRank");
        rankT[sol::meta_function::to_string] = [](const FactionRank& rec) -> std::string {
            return "ESM3_FactionRank[" + rec.mFactionId.toDebugString() + ", " + std::to_string(rec.mRankIndex + 1)
                + "]";
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
    }
}
