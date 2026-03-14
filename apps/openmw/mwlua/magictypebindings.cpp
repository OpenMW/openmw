#include "magictypebindings.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/lua/util.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

#include "types/usertypeutil.hpp"

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
}

namespace MWLua
{
    namespace
    {
        template <class T>
        void addEffectsProperty(sol::state_view& lua, sol::usertype<T>& record)
        {
            if constexpr (Types::RecordType<T>::isMutable)
            {
                //TODO
            }
            else
            {
                record["effects"] = sol::readonly_property([lua = lua.lua_state()](const T& rec) -> sol::table {
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
        auto effectParamsT = state.new_usertype<ESM::IndexedENAMstruct>("ESM3_EffectParams");
        effectParamsT[sol::meta_function::to_string] = [](const ESM::IndexedENAMstruct& params) {
            return std::format("ESM3_EffectParams[{}]", params.mData.mEffectID.toDebugString());
        };
        const auto* magicEffectStore = &MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>();
        effectParamsT["effect"] = sol::readonly_property(
            [magicEffectStore](const ESM::IndexedENAMstruct& params) -> const ESM::MagicEffect* {
                return magicEffectStore->find(params.mData.mEffectID);
            });
        effectParamsT["id"] = sol::readonly_property(
            [](const ESM::IndexedENAMstruct& params) -> ESM::RefId { return params.mData.mEffectID; });
        effectParamsT["affectedSkill"]
            = sol::readonly_property([](const ESM::IndexedENAMstruct& params) -> sol::optional<std::string> {
                  if (!params.mData.mSkill.empty())
                      return params.mData.mSkill.serializeText();
                  return sol::nullopt;
              });
        effectParamsT["affectedAttribute"]
            = sol::readonly_property([](const ESM::IndexedENAMstruct& params) -> sol::optional<std::string> {
                  if (!params.mData.mAttribute.empty())
                      return params.mData.mAttribute.serializeText();
                  return sol::nullopt;
              });
        effectParamsT["range"]
            = sol::readonly_property([](const ESM::IndexedENAMstruct& params) -> int { return params.mData.mRange; });
        effectParamsT["area"]
            = sol::readonly_property([](const ESM::IndexedENAMstruct& params) -> int { return params.mData.mArea; });
        effectParamsT["magnitudeMin"]
            = sol::readonly_property([](const ESM::IndexedENAMstruct& params) -> int { return params.mData.mMagnMin; });
        effectParamsT["magnitudeMax"]
            = sol::readonly_property([](const ESM::IndexedENAMstruct& params) -> int { return params.mData.mMagnMax; });
        effectParamsT["duration"] = sol::readonly_property(
            [](const ESM::IndexedENAMstruct& params) -> int { return params.mData.mDuration; });
        effectParamsT["index"]
            = sol::readonly_property([](const ESM::IndexedENAMstruct& params) -> int { return params.mIndex; });
    }

    void addPotionType(sol::state_view& lua)
    {
        addPotionType<ESM::Potion>(lua, "ESM3_Potion");
    }

    void addMutablePotionType(sol::state_view& lua)
    {
        addPotionType<MutableRecord<ESM::Potion>>(lua, "ESM3_MutablePotion");
    }
}
