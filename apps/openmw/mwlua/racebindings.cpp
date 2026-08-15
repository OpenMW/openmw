#include "racebindings.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/lua/luastate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

#include "idcollectionbindings.hpp"
#include "recordstore.hpp"
#include "types/usertypeutil.hpp"

#include <format>

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

    struct MutableRaceAttributes
    {
        MWLua::MutableRecord<ESM::Race> mRace;

        std::map<ESM::RefId, ESM::Race::AttributeValues>& find() { return mRace.find().mData.mAttributeValues; }

        const std::map<ESM::RefId, ESM::Race::AttributeValues>& find() const
        {
            return mRace.find().mData.mAttributeValues;
        }
    };

    struct IndexedMutableRaceAttributes
    {
        MWLua::MutableRecord<ESM::Race> mRace;
        ESM::RefId mAttribute;
    };

    struct RaceSpells
    {
        MWLua::MutableRecord<ESM::Race> mRace;

        std::vector<ESM::RefId>& find() { return mRace.find().mPowers.mList; }

        const std::vector<ESM::RefId>& find() const { return mRace.find().mPowers.mList; }
    };

    struct RaceSkills
    {
        MWLua::MutableRecord<ESM::Race> mRace;

        std::array<ESM::Race::SkillBonus, 7>& find() { return mRace.find().mData.mBonus; }

        const std::array<ESM::Race::SkillBonus, 7>& find() const { return mRace.find().mData.mBonus; }
    };

    enum class HeightWeight
    {
        Height,
        Weight
    };

    struct RaceHeightWeight
    {
        MWLua::MutableRecord<ESM::Race> mRace;
        HeightWeight mType;

        static float get(const ESM::Race& race, HeightWeight type, bool male)
        {
            if (type == HeightWeight::Height)
            {
                if (male)
                    return race.mData.mMaleHeight;
                return race.mData.mFemaleHeight;
            }
            if (male)
                return race.mData.mMaleWeight;
            return race.mData.mFemaleWeight;
        }

        static void set(ESM::Race& race, HeightWeight type, bool male, float value)
        {
            if (value < 0.1f || value > 2.f)
                throw std::runtime_error("value must be in the range 0.1-2.0");
            if (type == HeightWeight::Height)
            {
                if (male)
                    race.mData.mMaleHeight = value;
                else
                    race.mData.mFemaleHeight = value;
            }
            else
            {
                if (male)
                    race.mData.mMaleWeight = value;
                else
                    race.mData.mFemaleWeight = value;
            }
        }

        float get(bool male) const { return get(mRace.find(), mType, male); }

        void set(bool male, float value) { set(mRace.find(), mType, male, value); }
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
    template <>
    struct is_automagical<MutableRaceAttributes> : std::false_type
    {
    };
    template <>
    struct is_automagical<IndexedMutableRaceAttributes> : std::false_type
    {
    };
    template <>
    struct is_automagical<RaceSpells> : std::false_type
    {
    };
    template <>
    struct is_automagical<RaceSkills> : std::false_type
    {
    };
    template <>
    struct is_automagical<RaceHeightWeight> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace
    {
        void setFlagProperty(const sol::table& rec, std::string_view key, int32_t& flags, int flag)
        {
            if (rec[key] != sol::nil)
            {
                if (rec[key])
                    flags |= flag;
                else
                    flags &= ~flag;
            }
        }

        void setHWFromTable(ESM::Race& race, HeightWeight type, const sol::object& value)
        {
            if (value.is<RaceHeightWeight>())
            {
                const auto& other = value.as<RaceHeightWeight>();
                RaceHeightWeight::set(race, type, true, other.get(true));
                RaceHeightWeight::set(race, type, false, other.get(false));
            }
            else
            {
                auto table = value.as<sol::lua_table>();
                RaceHeightWeight::set(race, type, true, table.get_or("male", Misc::FiniteFloat(1.f)));
                RaceHeightWeight::set(race, type, false, table.get_or("female", Misc::FiniteFloat(1.f)));
            }
        }

        void setSpellsFromTable(ESM::Race& race, const sol::object& value)
        {
            if (value == sol::nil)
                race.mPowers.mList.clear();
            else if (value.is<RaceSpells>())
                race.mPowers.mList = value.as<RaceSpells>().find();
            else
            {
                auto table = value.as<sol::lua_table>();
                race.mPowers.mList.clear();
                const size_t length = table.size();
                race.mPowers.mList.reserve(length);
                for (size_t i = 0; i < length; ++i)
                    race.mPowers.mList.push_back(
                        ESM::RefId::deserializeText(table.get<std::string_view>(LuaUtil::toLuaIndex(i))));
            }
        }

        void setSkillsFromTable(ESM::Race& race, const sol::object& value)
        {
            if (value == sol::nil)
                race.mData.mBonus.fill({});
            else if (value.is<RaceSkills>())
            {
                const std::array<ESM::Race::SkillBonus, 7>& bonuses = value.as<RaceSkills>().find();
                for (size_t i = 0; i < race.mData.mBonus.size(); ++i)
                    race.mData.mBonus[i] = bonuses[i];
            }
            else
            {
                auto table = value.as<sol::lua_table>();
                auto it = table.begin();
                for (size_t i = 0; i < race.mData.mBonus.size(); ++i)
                {
                    if (it != table.end())
                    {
                        const auto& [k, v] = *it;
                        const ESM::RefId skill = ESM::RefId::deserializeText(k.as<std::string_view>());
                        const int bonus = v.as<int>();
                        race.mData.mBonus[i].mSkill = skill;
                        race.mData.mBonus[i].mBonus = bonus;
                        ++it;
                    }
                    else
                        race.mData.mBonus[i] = {};
                }
            }
        }

        void setAttributesFromTable(
            std::map<ESM::RefId, ESM::Race::AttributeValues>& values, ESM::RefId attribute, const sol::object& value)
        {
            if (value == sol::nil)
                values.erase(attribute);
            else if (value.is<IndexedMutableRaceAttributes>())
            {
                const auto& attributes = value.as<IndexedMutableRaceAttributes>();
                const auto& map = attributes.mRace.find().mData.mAttributeValues;
                const auto it = map.find(attributes.mAttribute);
                if (it != map.end())
                    values[attribute] = it->second;
                else
                    values.erase(attribute);
            }
            else
            {
                auto table = value.as<sol::lua_table>();
                auto& v = values[attribute];
                v.mFemale = table.get_or("female", 0);
                v.mMale = table.get_or("male", 0);
            }
        }

        void setAttributesFromTable(ESM::Race& race, const sol::object& value)
        {
            if (value == sol::nil)
                race.mData.mAttributeValues.clear();
            else if (value.is<MutableRaceAttributes>())
            {
                const ESM::Race& other = value.as<MutableRaceAttributes>().mRace.find();
                race.mData.mAttributeValues = other.mData.mAttributeValues;
            }
            else
            {
                auto table = value.as<sol::lua_table>();
                race.mData.mAttributeValues.clear();
                for (const auto& [id, values] : table)
                {
                    const ESM::RefId attribute = ESM::RefId::deserializeText(id.as<std::string_view>());
                    if (!attribute.empty())
                        setAttributesFromTable(race.mData.mAttributeValues, attribute, values);
                }
            }
        }

        template <class T>
        void addUserType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);
            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return std::format("ESM3_Race[{}]", rec.mId.toDebugString()); };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });
            Types::addProperty(record, "name", &ESM::Race::mName);
            Types::addProperty(record, "description", &ESM::Race::mDescription);
            Types::addFlagProperty(
                record, "isPlayable", ESM::Race::Playable, &ESM::Race::mData, &ESM::Race::RADTstruct::mFlags);
            Types::addFlagProperty(
                record, "isBeast", ESM::Race::Beast, &ESM::Race::mData, &ESM::Race::RADTstruct::mFlags);

            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["height"] = sol::property(
                    [](const MutableRecord<ESM::Race>& rec) {
                        return RaceHeightWeight{ rec, HeightWeight::Height };
                    },
                    [](MutableRecord<ESM::Race>& rec, const sol::object& value) {
                        ESM::Race& race = rec.find();
                        setHWFromTable(race, HeightWeight::Height, value);
                    });
                record["weight"] = sol::property(
                    [](const MutableRecord<ESM::Race>& rec) {
                        return RaceHeightWeight{ rec, HeightWeight::Weight };
                    },
                    [](MutableRecord<ESM::Race>& rec, const sol::object& value) {
                        ESM::Race& race = rec.find();
                        setHWFromTable(race, HeightWeight::Weight, value);
                    });
                auto hwT = lua.new_usertype<RaceHeightWeight>("ESM3_MutableRaceHeightWeight");
                hwT["male"] = sol::property(
                    [](const RaceHeightWeight& heightWeight) -> float { return heightWeight.get(true); },
                    [](RaceHeightWeight& heightWeight, Misc::FiniteFloat value) { heightWeight.set(true, value); });
                hwT["female"] = sol::property(
                    [](const RaceHeightWeight& heightWeight) -> float { return heightWeight.get(false); },
                    [](RaceHeightWeight& heightWeight, Misc::FiniteFloat value) { heightWeight.set(false, value); });

                record["spells"] = sol::property([](const MutableRecord<ESM::Race>& rec) { return RaceSpells{ rec }; },
                    [](MutableRecord<ESM::Race>& rec, const sol::object& value) {
                        ESM::Race& race = rec.find();
                        setSpellsFromTable(race, value);
                    });
                auto spellsT = lua.new_usertype<RaceSpells>("ESM3_MutableRaceSpells");
                spellsT[sol::meta_function::length] = [](const RaceSpells& spells) { return spells.find().size(); };
                spellsT[sol::meta_function::index]
                    = [](const RaceSpells& spells, uint32_t index) -> std::optional<ESM::RefId> {
                    const auto& vec = spells.find();
                    if (index == 0 || index > vec.size())
                        return {};
                    return vec[index - 1];
                };
                spellsT[sol::meta_function::new_index] = [](RaceSpells& spells, uint32_t i, const sol::object& value) {
                    auto& vec = spells.find();
                    if (i == 0 || i > vec.size() + 1)
                        throw std::runtime_error("index out of range");
                    --i;
                    if (value == sol::nil)
                    {
                        if (i >= vec.size())
                            return;
                        vec.erase(vec.begin() + i);
                        return;
                    }
                    const ESM::RefId id = ESM::RefId::deserializeText(value.as<std::string_view>());
                    if (i == vec.size())
                        vec.push_back(id);
                    else
                        vec[i] = id;
                };
                spellsT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
                spellsT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

                record["skills"] = sol::property([](const MutableRecord<ESM::Race>& rec) { return RaceSkills{ rec }; },
                    [](MutableRecord<ESM::Race>& rec, const sol::object& value) {
                        ESM::Race& race = rec.find();
                        setSkillsFromTable(race, value);
                    });
                auto skillsT = lua.new_usertype<RaceSkills>("ESM3_MutableRaceSkills");
                skillsT[sol::meta_function::index]
                    = [](const RaceSkills& skills, std::string_view index) -> std::optional<int> {
                    const auto& bonuses = skills.find();
                    const ESM::RefId skill = ESM::RefId::deserializeText(index);
                    if (skill.empty())
                        return {};
                    for (const auto& bonus : bonuses)
                    {
                        if (bonus.mSkill == skill)
                            return bonus.mBonus;
                    }
                    return {};
                };
                skillsT[sol::meta_function::new_index] = [](RaceSkills& skills, std::string_view index,
                                                             std::optional<int> value) {
                    auto& bonuses = skills.find();
                    const ESM::RefId skill = ESM::RefId::deserializeText(index);
                    if (skill.empty())
                        return;
                    if (!value)
                    {
                        for (auto& bonus : bonuses)
                        {
                            if (bonus.mSkill == skill)
                                bonus = {};
                        }
                        return;
                    }
                    bool set = false;
                    size_t firstEmpty = bonuses.size() + 1;
                    for (size_t i = 0; i < bonuses.size(); ++i)
                    {
                        if (bonuses[i].mSkill == skill)
                        {
                            if (set)
                                bonuses[i] = {};
                            else
                            {
                                set = true;
                                bonuses[i].mBonus = *value;
                            }
                        }
                        else if (bonuses[i].mSkill.empty())
                            firstEmpty = i;
                    }
                    if (set)
                        return;
                    if (firstEmpty < bonuses.size())
                    {
                        bonuses[firstEmpty].mSkill = skill;
                        bonuses[firstEmpty].mBonus = *value;
                    }
                    else
                        throw std::runtime_error(std::format("cannot add more than {} skill bonuses", bonuses.size()));
                };
                skillsT[sol::meta_function::pairs] = [&](sol::this_state ts, const RaceSkills& obj) {
                    return std::make_tuple(sol::as_function([index = size_t{}](const RaceSkills& skills) mutable
                                               -> std::pair<std::optional<ESM::RefId>, std::optional<int>> {
                        const auto& bonuses = skills.find();
                        while (index < bonuses.size())
                        {
                            const auto& bonus = bonuses[index];
                            ++index;
                            if (!bonus.mSkill.empty())
                                return { bonus.mSkill, bonus.mBonus };
                        }
                        return { {}, {} };
                    }),
                        obj, sol::nil);
                };

                record["attributes"]
                    = sol::property([](const MutableRecord<ESM::Race>& rec) { return MutableRaceAttributes{ rec }; },
                        [](MutableRecord<ESM::Race>& rec, const sol::object& value) {
                            ESM::Race& race = rec.find();
                            setAttributesFromTable(race, value);
                        });
                auto attributesT = lua.new_usertype<MutableRaceAttributes>("ESM3_MutableRaceAttributes");
                attributesT[sol::meta_function::length]
                    = [](const MutableRaceAttributes& attributes) { return attributes.find().size(); };
                attributesT[sol::meta_function::index]
                    = [](const MutableRaceAttributes& attributes,
                          std::string_view id) -> std::optional<IndexedMutableRaceAttributes> {
                    const ESM::RefId attribute = ESM::RefId::deserializeText(id);
                    if (attribute.empty())
                        return {};
                    return IndexedMutableRaceAttributes{ attributes.mRace, attribute };
                };
                attributesT[sol::meta_function::new_index]
                    = [](MutableRaceAttributes& attributes, std::string_view id, const sol::object& value) {
                          auto& values = attributes.find();
                          const ESM::RefId attribute = ESM::RefId::deserializeText(id);
                          if (attribute.empty())
                              return;
                          setAttributesFromTable(values, attribute, value);
                      };
                attributesT[sol::meta_function::pairs] = [](const MutableRaceAttributes& obj) {
                    return std::make_tuple(sol::as_function([](sol::this_state ts,
                                                                const MutableRaceAttributes& attributes,
                                                                const sol::object& key) mutable
                                               -> std::optional<std::tuple<ESM::RefId, IndexedMutableRaceAttributes>> {
                        const auto& map = attributes.find();
                        auto it = map.begin();
                        if (key != sol::nil)
                        {
                            it = map.find(ESM::RefId::deserializeText(key.as<std::string_view>()));
                            if (it != map.end())
                                ++it;
                        }
                        if (it == map.end())
                            return {};
                        return std::make_tuple(it->first, IndexedMutableRaceAttributes{ attributes.mRace, it->first });
                    }),
                        obj, sol::nil);
                };

                auto iAttrT = lua.new_usertype<IndexedMutableRaceAttributes>("ESM3_IndexedMutableRaceAttributes");
                iAttrT["male"] = sol::property(
                    [](const IndexedMutableRaceAttributes& attributes) -> std::optional<int> {
                        const auto& map = attributes.mRace.find().mData.mAttributeValues;
                        const auto it = map.find(attributes.mAttribute);
                        if (it != map.end())
                            return it->second.mMale;
                        return {};
                    },
                    [](IndexedMutableRaceAttributes& attributes, int value) {
                        auto& map = attributes.mRace.find().mData.mAttributeValues;
                        map[attributes.mAttribute].mMale = value;
                    });
                iAttrT["female"] = sol::property(
                    [](const IndexedMutableRaceAttributes& attributes) -> std::optional<int> {
                        const auto& map = attributes.mRace.find().mData.mAttributeValues;
                        const auto it = map.find(attributes.mAttribute);
                        if (it != map.end())
                            return it->second.mFemale;
                        return {};
                    },
                    [](IndexedMutableRaceAttributes& attributes, int value) {
                        auto& map = attributes.mRace.find().mData.mAttributeValues;
                        map[attributes.mAttribute].mFemale = value;
                    });
            }
            else
            {
                record["height"] = sol::readonly_property([lua](const ESM::Race& rec) -> sol::table {
                    sol::table res(lua, sol::create);
                    res["male"] = rec.mData.mMaleHeight;
                    res["female"] = rec.mData.mFemaleHeight;
                    return LuaUtil::makeReadOnly(res);
                });
                record["weight"] = sol::readonly_property([lua](const ESM::Race& rec) -> sol::table {
                    sol::table res(lua, sol::create);
                    res["male"] = rec.mData.mMaleWeight;
                    res["female"] = rec.mData.mFemaleWeight;
                    return LuaUtil::makeReadOnly(res);
                });
                record["spells"] = sol::readonly_property([lua](const ESM::Race& rec) -> sol::table {
                    return createReadOnlyRefIdTable(lua, rec.mPowers.mList);
                });
                record["skills"] = sol::readonly_property([lua](const ESM::Race& rec) -> sol::table {
                    sol::table res(lua, sol::create);
                    for (const auto& skillBonus : rec.mData.mBonus)
                    {
                        if (!skillBonus.mSkill.empty())
                            res[skillBonus.mSkill] = skillBonus.mBonus;
                    }
                    return res;
                });
                record["attributes"] = sol::readonly_property([lua](const ESM::Race& rec) -> RaceAttributes {
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
            }
        }
    }

    ESM::Race tableToRace(const sol::table& rec)
    {
        auto race = Types::initFromTemplate<ESM::Race>(rec);
        if (rec["name"] != sol::nil)
            race.mName = rec["name"];
        if (rec["description"] != sol::nil)
            race.mDescription = rec["description"];
        if (rec["spells"] != sol::nil)
            setSpellsFromTable(race, rec["spells"]);
        if (rec["skills"] != sol::nil)
            setSkillsFromTable(race, rec["skills"]);
        setFlagProperty(rec, "isPlayable", race.mData.mFlags, ESM::Race::Playable);
        setFlagProperty(rec, "isBeast", race.mData.mFlags, ESM::Race::Beast);
        if (rec["height"] != sol::nil)
            setHWFromTable(race, HeightWeight::Height, rec["height"]);
        if (rec["weight"] != sol::nil)
            setHWFromTable(race, HeightWeight::Weight, rec["weight"]);
        if (rec["attributes"] != sol::nil)
            setAttributesFromTable(race, rec["attributes"]);
        return race;
    }

    void addMutableRaceType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Race>>(lua, "ESM3_MutableRace");
    }

    sol::table initRaceRecordBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table races(lua, sol::create);
        addRecordFunctionBinding<ESM::Race>(races, context);
        addUserType<ESM::Race>(lua, "ESM3_Race");
        return LuaUtil::makeReadOnly(races);
    }
}
