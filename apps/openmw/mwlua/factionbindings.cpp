#include "factionbindings.hpp"
#include "recordstore.hpp"
#include "types/usertypeutil.hpp"

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

    struct MutableFactionRanks
    {
        MWLua::MutableRecord<ESM::Faction> mFaction;
    };

    struct MutableFactionRank
    {
        MWLua::MutableRecord<ESM::Faction> mFaction;
        uint32_t mIndex;
    };

    struct MutableFactionRankAttributes
    {
        MWLua::MutableRecord<ESM::Faction> mFaction;
        uint32_t mIndex;

        ESM::RankData& find() { return mFaction.find().mData.mRankData[mIndex]; }

        const ESM::RankData& find() const { return mFaction.find().mData.mRankData[mIndex]; }
    };

    struct MutableFactionReactions
    {
        MWLua::MutableRecord<ESM::Faction> mFaction;

        std::map<ESM::RefId, int32_t>& find() { return mFaction.find().mReactions; }

        const std::map<ESM::RefId, int32_t>& find() const { return mFaction.find().mReactions; }
    };

    template <size_t N>
    using RefIdArrayAccessor = std::array<ESM::RefId, N> ESM::Faction::FADTstruct::*;

    template <size_t N>
    struct MutableFactionArray
    {
        MWLua::MutableRecord<ESM::Faction> mFaction;
        RefIdArrayAccessor<N> mAccessor;

        std::array<ESM::RefId, N>& find() { return mFaction.find().mData.*mAccessor; }

        const std::array<ESM::RefId, N>& find() const { return mFaction.find().mData.*mAccessor; }
    };
}

namespace sol
{
    template <>
    struct is_automagical<ESM::Faction> : std::false_type
    {
    };
    template <>
    struct is_automagical<FactionRank> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWWorld::Store<FactionRank>> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableFactionRanks> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableFactionRank> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableFactionRankAttributes> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableFactionReactions> : std::false_type
    {
    };
    template <size_t T>
    struct is_automagical<MutableFactionArray<T>> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace
    {
        void setFromTable(int32_t& attribute1, int32_t& attribute2, const sol::object& value)
        {
            if (value == sol::nil)
            {
                attribute1 = 0;
                attribute2 = 0;
            }
            else if (value.is<MutableFactionRankAttributes>())
            {
                const ESM::RankData& data = value.as<MutableFactionRankAttributes>().find();
                attribute1 = data.mAttribute1;
                attribute2 = data.mAttribute2;
            }
            else
            {
                auto attributeValues = value.as<sol::lua_table>();
                const size_t length = attributeValues.size();
                attribute1 = 0;
                attribute2 = 0;
                if (length > 0)
                    attribute1 = attributeValues[1];
                if (length > 1)
                    attribute2 = attributeValues[2];
            }
        }

        void setFromTable(std::string& rank, ESM::RankData& rankData, const sol::lua_table& table)
        {
            rank = table.get_or<std::string_view>("name", {});
            rankData.mPrimarySkill = table.get_or("primarySkillValue", 0);
            rankData.mFavouredSkill = table.get_or("favouredSkillValue", 0);
            rankData.mFactReputation = table.get_or("factionReputation", 0);
            setFromTable(rankData.mAttribute1, rankData.mAttribute2, table["attributeValues"]);
        }

        void setFromTable(
            std::array<std::string, 10>& ranks, std::array<ESM::RankData, 10>& rankData, const sol::lua_table& table)
        {
            const size_t length = table.size();
            for (size_t i = 0; i < ranks.size(); ++i)
            {
                if (i >= length)
                {
                    ranks[i].clear();
                    rankData[i].blank();
                }
                else
                    setFromTable(ranks[i], rankData[i], table[LuaUtil::toLuaIndex(i)]);
            }
        }

        void setFromTable(std::map<ESM::RefId, int32_t>& reactions, const sol::lua_table& table)
        {
            reactions.clear();
            for (const auto& [key, reaction] : table)
            {
                const ESM::RefId id = ESM::RefId::deserializeText(key.as<std::string_view>());
                if (!id.empty())
                    reactions[id] = reaction.as<int>();
            }
        }

        template <size_t N>
        void setFromTable(std::array<ESM::RefId, N>& ids, const sol::lua_table& table)
        {
            const size_t length = table.size();
            for (uint32_t i = 0; i < N; ++i)
            {
                if (i >= length)
                    ids[i] = {};
                else
                    ids[i] = ESM::RefId::deserializeText(table.get<std::string_view>(LuaUtil::toLuaIndex(i)));
            }
        }

        auto addMutableFactionRanksType(sol::state_view& lua)
        {
            auto attrT = lua.new_usertype<MutableFactionRankAttributes>("ESM3_MutableFactionRankAttributes");
            attrT[sol::meta_function::length] = [](const MutableFactionRankAttributes&) { return 2; };
            attrT[sol::meta_function::index]
                = [](const MutableFactionRankAttributes& attributes, uint32_t index) -> std::optional<int32_t> {
                const ESM::RankData& data = attributes.find();
                if (index == 1)
                    return data.mAttribute1;
                else if (index == 2)
                    return data.mAttribute2;
                return {};
            };
            attrT[sol::meta_function::new_index]
                = [](MutableFactionRankAttributes& attributes, uint32_t index, int32_t value) {
                      ESM::RankData& data = attributes.find();
                      if (index == 1)
                          data.mAttribute1 = value;
                      else if (index == 2)
                          data.mAttribute2 = value;
                      else
                          throw std::runtime_error("index out of range");
                  };
            attrT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
            attrT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

            auto rankT = lua.new_usertype<MutableFactionRank>("ESM3_MutableFactionRank");
            rankT["name"] = sol::property(
                [](const MutableFactionRank& rec) -> std::string_view {
                    return rec.mFaction.find().mRanks[rec.mIndex];
                },
                [](MutableFactionRank& rec, std::string_view value) {
                    rec.mFaction.find().mRanks[rec.mIndex] = value;
                });
            rankT["primarySkillValue"] = sol::property(
                [](const MutableFactionRank& rec) -> int {
                    return rec.mFaction.find().mData.mRankData[rec.mIndex].mPrimarySkill;
                },
                [](MutableFactionRank& rec, int value) {
                    rec.mFaction.find().mData.mRankData[rec.mIndex].mPrimarySkill = value;
                });
            rankT["favouredSkillValue"] = sol::property(
                [](const MutableFactionRank& rec) -> int {
                    return rec.mFaction.find().mData.mRankData[rec.mIndex].mFavouredSkill;
                },
                [](MutableFactionRank& rec, int value) {
                    rec.mFaction.find().mData.mRankData[rec.mIndex].mFavouredSkill = value;
                });
            rankT["factionReputation"] = sol::property(
                [](const MutableFactionRank& rec) -> int {
                    return rec.mFaction.find().mData.mRankData[rec.mIndex].mFactReputation;
                },
                [](MutableFactionRank& rec, int value) {
                    rec.mFaction.find().mData.mRankData[rec.mIndex].mFactReputation = value;
                });
            rankT["attributeValues"] = sol::property(
                [](const MutableFactionRank& rec) {
                    return MutableFactionRankAttributes{ rec.mFaction, rec.mIndex };
                },
                [](MutableFactionRank& rec, const sol::object& value) {
                    ESM::RankData& data = rec.mFaction.find().mData.mRankData[rec.mIndex];
                    setFromTable(data.mAttribute1, data.mAttribute2, value);
                });

            auto ranksT = lua.new_usertype<MutableFactionRanks>("ESM3_MutableFactionRanks");
            ranksT[sol::meta_function::length]
                = [](const MutableFactionRanks& ranks) { return ranks.mFaction.find().mRanks.size(); };
            ranksT[sol::meta_function::index]
                = [](const MutableFactionRanks& ranks, uint32_t index) -> std::optional<MutableFactionRank> {
                const ESM::Faction& faction = ranks.mFaction.find();
                if (index == 0 || index > faction.mRanks.size())
                    return {};
                return MutableFactionRank{ ranks.mFaction, index - 1 };
            };
            ranksT[sol::meta_function::new_index]
                = [](MutableFactionRanks& ranks, uint32_t i, const sol::object& value) {
                      ESM::Faction& faction = ranks.mFaction.find();
                      if (i == 0 || i > faction.mRanks.size())
                          throw std::runtime_error("index out of range");
                      --i;
                      if (value == sol::nil)
                      {
                          faction.mRanks[i].clear();
                          faction.mData.mRankData[i].blank();
                      }
                      else if (value.is<MutableFactionRank>())
                      {
                          const auto& rank = value.as<MutableFactionRank>();
                          const ESM::Faction& otherFaction = rank.mFaction.find();
                          faction.mRanks[i] = otherFaction.mRanks[rank.mIndex];
                          faction.mData.mRankData[i] = otherFaction.mData.mRankData[rank.mIndex];
                      }
                      else
                          setFromTable(faction.mRanks[i], faction.mData.mRankData[i], value.as<sol::lua_table>());
                  };
            ranksT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
            ranksT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

            return sol::property([](const MutableRecord<ESM::Faction>& rec) { return MutableFactionRanks{ rec }; },
                [](MutableRecord<ESM::Faction>& rec, const sol::object& value) {
                    ESM::Faction& faction = rec.find();
                    if (value == sol::nil)
                    {
                        faction.mRanks.fill({});
                        for (ESM::RankData& data : faction.mData.mRankData)
                            data.blank();
                    }
                    else if (value.is<MutableFactionRanks>())
                    {
                        const ESM::Faction& other = value.as<MutableFactionRanks>().mFaction.find();
                        assert(other.mData.mRankData.size() == other.mRanks.size());
                        for (size_t i = 0; i < faction.mRanks.size(); ++i)
                        {
                            faction.mRanks[i] = other.mRanks[i];
                            faction.mData.mRankData[i] = other.mData.mRankData[i];
                        }
                    }
                    else
                        setFromTable(faction.mRanks, faction.mData.mRankData, value.as<sol::lua_table>());
                });
        }

        auto addMutableFactionReactionsType(sol::state_view& lua)
        {
            auto record = lua.new_usertype<MutableFactionReactions>("ESM3_MutableFactionReactions");
            record[sol::meta_function::length]
                = [](const MutableFactionReactions& reactions) { return reactions.find().size(); };
            record[sol::meta_function::index]
                = [](const MutableFactionReactions& reactions, std::string_view id) -> std::optional<int32_t> {
                const auto& map = reactions.find();
                const auto it = map.find(ESM::RefId::deserializeText(id));
                if (it != map.end())
                    return it->second;
                return {};
            };
            record[sol::meta_function::new_index]
                = [](MutableFactionReactions& reactions, std::string_view id, const sol::object& value) {
                      auto& map = reactions.find();
                      const ESM::RefId faction = ESM::RefId::deserializeText(id);
                      if (value == sol::nil)
                          map.erase(faction);
                      else
                          map[faction] = value.as<int32_t>();
                  };
            record[sol::meta_function::pairs] = [](const MutableFactionReactions& obj) {
                return std::make_tuple(
                    sol::as_function(
                        [](sol::this_state ts, const MutableFactionReactions& reactions,
                            const sol::object& key) mutable -> std::optional<std::tuple<ESM::RefId, int32_t>> {
                            const auto& map = reactions.find();
                            auto it = map.begin();
                            if (key != sol::nil)
                            {
                                it = map.find(ESM::RefId::deserializeText(key.as<std::string_view>()));
                                if (it != map.end())
                                    ++it;
                            }
                            if (it == map.end())
                                return {};
                            return std::make_tuple(it->first, it->second);
                        }),
                    obj, sol::nil);
            };

            return sol::property([](const MutableRecord<ESM::Faction>& rec) { return MutableFactionReactions{ rec }; },
                [](MutableRecord<ESM::Faction>& rec, const sol::object& value) {
                    auto& reactions = rec.find().mReactions;
                    if (value == sol::nil)
                        reactions.clear();
                    else if (value.is<MutableFactionReactions>())
                        reactions = value.as<MutableFactionReactions>().find();
                    else
                        setFromTable(reactions, value.as<sol::lua_table>());
                });
        }

        template <size_t N>
        auto addMutableRefIdArrayType(sol::state_view& lua, std::string_view name, RefIdArrayAccessor<N> accessor)
        {
            using Array = MutableFactionArray<N>;
            auto arrayT = lua.new_usertype<Array>(name);
            arrayT[sol::meta_function::length] = [](const Array& array) { return array.find().size(); };
            arrayT[sol::meta_function::index] = [](const Array& array, uint32_t index) -> std::optional<ESM::RefId> {
                const auto& ids = array.find();
                if (index == 0 || index > ids.size())
                    return {};
                return ids[index - 1];
            };
            arrayT[sol::meta_function::new_index] = [](Array& array, uint32_t i, const sol::object& value) {
                auto& ids = array.find();
                if (i == 0 || i > ids.size())
                    throw std::runtime_error("index out of range");
                --i;
                if (value == sol::nil)
                    ids[i] = {};
                else
                    ids[i] = ESM::RefId::deserializeText(value.as<std::string_view>());
            };
            arrayT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
            arrayT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

            return sol::property(
                [=](const MutableRecord<ESM::Faction>& rec) {
                    return Array{ rec, accessor };
                },
                [=](MutableRecord<ESM::Faction>& rec, const sol::object& value) {
                    std::array<ESM::RefId, N>& ids = rec.find().mData.*accessor;
                    if (value == sol::nil)
                        ids.fill({});
                    else if (value.is<Array>())
                    {
                        const auto& other = value.as<Array>().find();
                        for (size_t i = 0; i < N; ++i)
                            ids[i] = other[i];
                    }
                    else
                        setFromTable(ids, value.as<sol::lua_table>());
                });
        }

        template <class T>
        void addUserType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);
            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Faction[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });
            Types::addProperty(record, "name", &ESM::Faction::mName);
            Types::addFlagProperty(
                record, "hidden", ESM::Faction::Hidden, &ESM::Faction::mData, &ESM::Faction::FADTstruct::mFlags);

            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["ranks"] = addMutableFactionRanksType(lua);
                record["reactions"] = addMutableFactionReactionsType(lua);
                record["attributes"] = addMutableRefIdArrayType(
                    lua, "ESM3_MutableFactionAttributes", &ESM::Faction::FADTstruct::mAttribute);
                record["skills"]
                    = addMutableRefIdArrayType(lua, "ESM3_MutableFactionSkills", &ESM::Faction::FADTstruct::mSkills);
            }
            else
            {
                record["ranks"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Faction& rec) {
                    sol::table res(lua, sol::create);
                    for (size_t i = 0; i < rec.mRanks.size() && i < rec.mData.mRankData.size(); i++)
                    {
                        if (rec.mRanks[i].empty())
                            break;

                        res.add(FactionRank(rec.mId, rec.mData.mRankData[i], rec.mRanks[i], i));
                    }

                    return res;
                });
                record["reactions"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Faction& rec) {
                    sol::table res(lua, sol::create);
                    for (const auto& [factionId, reaction] : rec.mReactions)
                        res[factionId] = reaction;

                    const auto* overrides
                        = MWBase::Environment::get().getDialogueManager()->getFactionReactionOverrides(rec.mId);

                    if (overrides != nullptr)
                    {
                        for (const auto& [factionId, reaction] : *overrides)
                            res[factionId] = reaction;
                    }

                    return res;
                });
                record["attributes"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Faction& rec) {
                    return createReadOnlyRefIdTable(lua, rec.mData.mAttribute);
                });
                record["skills"] = sol::readonly_property([lua = lua.lua_state()](const ESM::Faction& rec) {
                    return createReadOnlyRefIdTable(lua, rec.mData.mSkills);
                });
            }
        }
    }

    ESM::Faction tableToFaction(const sol::table& rec)
    {
        auto faction = Types::initFromTemplate<ESM::Faction>(rec);
        if (rec["name"] != sol::nil)
            faction.mName = rec["name"];
        if (rec["hidden"] != sol::nil)
        {
            if (rec["hidden"])
                faction.mData.mFlags |= ESM::Faction::Hidden;
            else
                faction.mData.mFlags &= ~ESM::Faction::Hidden;
        }
        if (rec["ranks"] != sol::nil)
            setFromTable(faction.mRanks, faction.mData.mRankData, rec["ranks"]);
        if (rec["reactions"] != sol::nil)
            setFromTable(faction.mReactions, rec["reactions"]);
        if (rec["attributes"] != sol::nil)
            setFromTable(faction.mData.mAttribute, rec["attributes"]);
        if (rec["skills"] != sol::nil)
            setFromTable(faction.mData.mSkills, rec["skills"]);
        return faction;
    }

    void addMutableFactionType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Faction>>(lua, "ESM3_MutableFaction");
    }

    sol::table initCoreFactionBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table factions(lua, sol::create);
        addRecordFunctionBinding<ESM::Faction>(factions, context);
        // Faction record
        addUserType<ESM::Faction>(lua, "ESM3_Faction");
        auto rankT = lua.new_usertype<FactionRank>("ESM3_FactionRank");
        rankT[sol::meta_function::to_string] = [](const FactionRank& rec) -> std::string {
            return "ESM3_FactionRank[" + rec.mFactionId.toDebugString() + ", "
                + std::to_string(LuaUtil::toLuaIndex(rec.mRankIndex)) + "]";
        };
        rankT["name"]
            = sol::readonly_property([](const FactionRank& rec) -> std::string_view { return rec.mRankName; });
        rankT["primarySkillValue"]
            = sol::readonly_property([](const FactionRank& rec) -> int { return rec.mPrimarySkill; });
        rankT["favouredSkillValue"]
            = sol::readonly_property([](const FactionRank& rec) -> int { return rec.mFavouredSkill; });
        rankT["factionReputation"]
            = sol::readonly_property([](const FactionRank& rec) -> int { return rec.mFactReputation; });
        // deprecated
        rankT["factionReaction"]
            = sol::readonly_property([](const FactionRank& rec) -> int { return rec.mFactReputation; });
        rankT["attributeValues"] = sol::readonly_property([lua = lua.lua_state()](const FactionRank& rec) {
            sol::table res(lua, sol::create);
            res.add(rec.mAttribute1);
            res.add(rec.mAttribute2);
            return res;
        });
        return LuaUtil::makeReadOnly(factions);
    }
}
