#include "magictypebindings.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/lua/util.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

#include "types/usertypeutil.hpp"

#include <format>
#include <variant>

namespace
{
    struct MutableEffectsList
    {
        std::variant<MWLua::MutableRecord<ESM::Enchantment>, MWLua::MutableRecord<ESM::Potion>,
            MWLua::MutableRecord<ESM::Spell>>
            mRecord;

        ESM::EffectList& find()
        {
            return std::visit([](auto& rec) -> ESM::EffectList& { return rec.find().mEffects; }, mRecord);
        }

        const ESM::EffectList& find() const
        {
            return std::visit([](const auto& rec) -> const ESM::EffectList& { return rec.find().mEffects; }, mRecord);
        }
    };

    struct MutableENAMstruct
    {
        MutableEffectsList mList;
        uint32_t mIndex;

        ESM::IndexedENAMstruct& find() { return mList.find().mList.at(mIndex); }

        const ESM::IndexedENAMstruct& find() const { return mList.find().mList.at(mIndex); }
    };
}

namespace sol
{
    template <>
    struct is_automagical<ESM::Enchantment> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::IndexedENAMstruct> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::Potion> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::Spell> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableEffectsList> : std::false_type
    {
    };
    template <>
    struct is_automagical<MutableENAMstruct> : std::false_type
    {
    };
}

namespace MWLua
{
    template <>
    struct Types::RecordType<MutableENAMstruct>
    {
        using Record = ESM::IndexedENAMstruct;
        constexpr static bool isMutable = true;

        static const Record& asRecord(const MutableENAMstruct& rec) { return rec.find(); }
    };

    namespace
    {
        template <class T>
        void addEffectsProperty(sol::state_view& lua, sol::usertype<T>& type)
        {
            if constexpr (Types::RecordType<T>::isMutable)
            {
                type["effects"] = sol::property([](const T& rec) { return MutableEffectsList{ rec }; },
                    [](T& rec, const sol::object& value) {
                        auto& record = rec.find();
                        if (value == sol::nil)
                            record.mEffects.mList.clear();
                        else if (value.is<MutableEffectsList>())
                            record.mEffects = value.as<MutableEffectsList>().find();
                        else
                            record.mEffects = tableToEffectList(value);
                    });
            }
            else
            {
                type["effects"] = sol::readonly_property([lua = lua.lua_state()](const T& rec) -> sol::table {
                    sol::table res(lua, sol::create);
                    for (size_t i = 0; i < rec.mEffects.mList.size(); ++i)
                        res[LuaUtil::toLuaIndex(i)] = rec.mEffects.mList[i]; // ESM::IndexedENAMstruct (effect params)
                    return res;
                });
            }
        }

        template <class T>
        void addPotionType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Potion[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Potion::mName);
            Types::addModelProperty(record);
            Types::addIconProperty(record);
            Types::addProperty(record, "mwscript", &ESM::Potion::mScript);
            Types::addProperty(record, "weight", &ESM::Potion::mData, &ESM::Potion::ALDTstruct::mWeight);
            Types::addProperty(record, "value", &ESM::Potion::mData, &ESM::Potion::ALDTstruct::mValue);
            Types::addFlagProperty(
                record, "isAutocalc", ESM::Potion::Autocalc, &ESM::Potion::mData, &ESM::Potion::ALDTstruct::mFlags);
            addEffectsProperty(lua, record);
        }

        void addPropertyFromTable(const sol::lua_table& rec, std::string_view key, ESM::RefId& value)
        {
            if (rec[key] != sol::nil)
            {
                std::string_view id = rec[key].get<std::string_view>();
                value = ESM::RefId::deserializeText(id);
            }
        }

        ESM::IndexedENAMstruct tableToEnamStruct(const sol::object& value)
        {
            if (value.is<MutableENAMstruct>())
                return value.as<MutableENAMstruct>().find();
            else if (value.is<ESM::IndexedENAMstruct>())
                return value.as<ESM::IndexedENAMstruct>();
            sol::lua_table table = value.as<sol::lua_table>();
            ESM::IndexedENAMstruct out;
            addPropertyFromTable(table, "id", out.mData.mEffectID);
            const auto& store = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>();
            if (!store.search(out.mData.mEffectID))
                throw std::runtime_error("EffectParams require a valid magic effect id");
            addPropertyFromTable(table, "affectedSkill", out.mData.mSkill);
            addPropertyFromTable(table, "affectedAttribute", out.mData.mAttribute);
            out.mData.mRange = table.get_or("range", 0);
            if (out.mData.mRange != ESM::RT_Self && out.mData.mRange != ESM::RT_Touch
                && out.mData.mRange != ESM::RT_Target)
                throw std::runtime_error("invalid range type");
            out.mData.mArea = std::max(table.get_or("area", 0), 0);
            out.mData.mMagnMin = table.get_or("magnitudeMin", 0);
            out.mData.mMagnMax = std::max(table.get_or("magnitudeMax", 0), out.mData.mMagnMin);
            out.mData.mDuration = std::max(table.get_or("duration", 0), 0);
            out.mIndex = 0;
            return out;
        }

        template <class T>
        void addEffectParamsBindings(sol::state_view& lua, std::string_view name)
        {
            auto effectParamsT = lua.new_usertype<T>(name);
            if constexpr (!Types::RecordType<T>::isMutable)
            {
                effectParamsT[sol::meta_function::to_string] = [](const ESM::IndexedENAMstruct& params) {
                    return std::format("ESM3_EffectParams[{}]", params.mData.mEffectID.toDebugString());
                };
                effectParamsT["effect"]
                    = sol::readonly_property([](const ESM::IndexedENAMstruct& params) -> const ESM::MagicEffect* {
                          const auto& store = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>();
                          return store.find(params.mData.mEffectID);
                      });
            }
            Types::addProperty(effectParamsT, "id", &ESM::IndexedENAMstruct::mData, &ESM::ENAMstruct::mEffectID);
            Types::addProperty(
                effectParamsT, "affectedSkill", &ESM::IndexedENAMstruct::mData, &ESM::ENAMstruct::mSkill);
            Types::addProperty(
                effectParamsT, "affectedAttribute", &ESM::IndexedENAMstruct::mData, &ESM::ENAMstruct::mAttribute);
            Types::addProperty(effectParamsT, "range", &ESM::IndexedENAMstruct::mData, &ESM::ENAMstruct::mRange);
            Types::addProperty(effectParamsT, "area", &ESM::IndexedENAMstruct::mData, &ESM::ENAMstruct::mArea);
            Types::addProperty(
                effectParamsT, "magnitudeMin", &ESM::IndexedENAMstruct::mData, &ESM::ENAMstruct::mMagnMin);
            Types::addProperty(
                effectParamsT, "magnitudeMax", &ESM::IndexedENAMstruct::mData, &ESM::ENAMstruct::mMagnMax);
            Types::addProperty(effectParamsT, "duration", &ESM::IndexedENAMstruct::mData, &ESM::ENAMstruct::mDuration);
            effectParamsT["index"] = sol::readonly_property([](const T& params) { return params.mIndex; });
        }

        void addEffectsListBindings(sol::state_view& lua)
        {
            auto listType = lua.new_usertype<MutableEffectsList>("ESM3_MutableEffectsList");
            listType[sol::meta_function::length]
                = [](const MutableEffectsList& list) { return list.find().mList.size(); };
            listType[sol::meta_function::index]
                = [](const MutableEffectsList& list, uint32_t index) -> std::optional<MutableENAMstruct> {
                const ESM::EffectList& effects = list.find();
                if (index == 0 || index > effects.mList.size())
                    return {};
                return MutableENAMstruct{ list, index - 1 };
            };
            listType[sol::meta_function::new_index]
                = [](MutableEffectsList& list, uint32_t i, const sol::object& value) {
                      ESM::EffectList& effects = list.find();
                      if (i == 0 || i > effects.mList.size() + 1)
                          throw std::runtime_error("index out of range");
                      --i;
                      if (value == sol::nil)
                      {
                          if (i >= effects.mList.size())
                              return;
                          effects.mList.erase(effects.mList.begin() + i);
                      }
                      else if (i == effects.mList.size())
                          effects.mList.emplace_back(tableToEnamStruct(value));
                      else
                          effects.mList[i] = tableToEnamStruct(value);
                      effects.updateIndexes();
                  };
            listType[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
            listType[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

            addEffectParamsBindings<MutableENAMstruct>(lua, "ESM3_MutableEffectParams");
        }
    }

    void addSpellBindings(sol::state_view& state)
    {
        auto spellT = state.new_usertype<ESM::Spell>("ESM3_Spell");
        spellT[sol::meta_function::to_string]
            = [](const ESM::Spell& rec) -> std::string { return "ESM3_Spell[" + rec.mId.toDebugString() + "]"; };
        spellT["id"] = sol::readonly_property([](const ESM::Spell& rec) { return rec.mId.serializeText(); });
        spellT["name"] = sol::readonly_property([](const ESM::Spell& rec) -> std::string_view { return rec.mName; });
        spellT["type"] = sol::readonly_property([](const ESM::Spell& rec) -> int { return rec.mData.mType; });
        spellT["cost"] = sol::readonly_property([](const ESM::Spell& rec) -> int { return rec.mData.mCost; });
        spellT["alwaysSucceedFlag"] = sol::readonly_property(
            [](const ESM::Spell& rec) -> bool { return !!(rec.mData.mFlags & ESM::Spell::F_Always); });
        spellT["starterSpellFlag"] = sol::readonly_property(
            [](const ESM::Spell& rec) -> bool { return !!(rec.mData.mFlags & ESM::Spell::F_PCStart); });
        // Deprecated for consistency with other record types
        spellT["autocalcFlag"] = sol::readonly_property(
            [](const ESM::Spell& rec) -> bool { return !!(rec.mData.mFlags & ESM::Spell::F_Autocalc); });
        spellT["isAutocalc"] = sol::readonly_property(
            [](const ESM::Spell& rec) -> bool { return !!(rec.mData.mFlags & ESM::Spell::F_Autocalc); });
        addEffectsProperty(state, spellT);
    }

    void addEnchantmentBindings(sol::state_view& state)
    {
        auto enchantT = state.new_usertype<ESM::Enchantment>("ESM3_Enchantment");
        enchantT[sol::meta_function::to_string] = [](const ESM::Enchantment& rec) -> std::string {
            return "ESM3_Enchantment[" + rec.mId.toDebugString() + "]";
        };
        enchantT["id"] = sol::readonly_property([](const ESM::Enchantment& rec) { return rec.mId.serializeText(); });
        enchantT["type"] = sol::readonly_property([](const ESM::Enchantment& rec) -> int { return rec.mData.mType; });
        // Deprecated for consistency with other record types
        enchantT["autocalcFlag"] = sol::readonly_property(
            [](const ESM::Enchantment& rec) -> bool { return !!(rec.mData.mFlags & ESM::Enchantment::Autocalc); });
        enchantT["isAutocalc"] = sol::readonly_property(
            [](const ESM::Enchantment& rec) -> bool { return !!(rec.mData.mFlags & ESM::Enchantment::Autocalc); });
        enchantT["cost"] = sol::readonly_property([](const ESM::Enchantment& rec) -> int { return rec.mData.mCost; });
        enchantT["charge"]
            = sol::readonly_property([](const ESM::Enchantment& rec) -> int { return rec.mData.mCharge; });
        addEffectsProperty(state, enchantT);
    }

    void addEffectParamsBindings(sol::state_view& state)
    {
        addEffectParamsBindings<ESM::IndexedENAMstruct>(state, "ESM3_EffectParams");
    }

    void addPotionType(sol::state_view& lua)
    {
        addPotionType<ESM::Potion>(lua, "ESM3_Potion");
    }

    void addMutablePotionType(sol::state_view& lua)
    {
        addPotionType<MutableRecord<ESM::Potion>>(lua, "ESM3_MutablePotion");
        addEffectsListBindings(lua);
    }

    ESM::EffectList tableToEffectList(const sol::table& effectsTable)
    {
        size_t numEffects = effectsTable.size();
        ESM::EffectList out;
        out.mList.resize(numEffects);
        for (size_t i = 0; i < numEffects; ++i)
        {
            out.mList[i] = tableToEnamStruct(effectsTable[LuaUtil::toLuaIndex(i)]);
        }
        out.updateIndexes();
        return out;
    }
}
