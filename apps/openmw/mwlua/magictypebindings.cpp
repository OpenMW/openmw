#include "magictypebindings.hpp"

#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/lua/util.hpp>
#include <components/misc/color.hpp>

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
    struct is_automagical<ESM::MagicEffect> : std::false_type
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

        void addPropertyFromTable(const sol::table& rec, std::string_view key, ESM::RefId& value)
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

        template <class T>
        void addSpellType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Spell[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "name", &ESM::Spell::mName);
            Types::addProperty(record, "type", &ESM::Spell::mData, &ESM::Spell::SPDTstruct::mType);
            Types::addProperty(record, "cost", &ESM::Spell::mData, &ESM::Spell::SPDTstruct::mCost);
            Types::addFlagProperty(
                record, "alwaysSucceedFlag", ESM::Spell::F_Always, &ESM::Spell::mData, &ESM::Spell::SPDTstruct::mFlags);
            Types::addFlagProperty(
                record, "starterSpellFlag", ESM::Spell::F_PCStart, &ESM::Spell::mData, &ESM::Spell::SPDTstruct::mFlags);
            if constexpr (!Types::RecordType<T>::isMutable)
            {
                // Deprecated for consistency with other record types
                Types::addFlagProperty(record, "autocalcFlag", ESM::Spell::F_Autocalc, &ESM::Spell::mData,
                    &ESM::Spell::SPDTstruct::mFlags);
            }
            Types::addFlagProperty(
                record, "isAutocalc", ESM::Spell::F_Autocalc, &ESM::Spell::mData, &ESM::Spell::SPDTstruct::mFlags);
            addEffectsProperty(lua, record);
        }

        template <class T>
        void addEnchantmentType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Enchantment[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addProperty(record, "type", &ESM::Enchantment::mData, &ESM::Enchantment::ENDTstruct::mType);
            Types::addProperty(record, "cost", &ESM::Enchantment::mData, &ESM::Enchantment::ENDTstruct::mCost);
            Types::addProperty(record, "charge", &ESM::Enchantment::mData, &ESM::Enchantment::ENDTstruct::mCharge);
            if constexpr (!Types::RecordType<T>::isMutable)
            {
                // Deprecated for consistency with other record types
                Types::addFlagProperty(record, "autocalcFlag", ESM::Enchantment::Autocalc, &ESM::Enchantment::mData,
                    &ESM::Enchantment::ENDTstruct::mFlags);
            }
            Types::addFlagProperty(record, "isAutocalc", ESM::Enchantment::Autocalc, &ESM::Enchantment::mData,
                &ESM::Enchantment::ENDTstruct::mFlags);
            addEffectsProperty(lua, record);
        }

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

        void setReverseFlagProperty(const sol::table& rec, std::string_view key, int32_t& flags, int flag)
        {
            if (rec[key] != sol::nil)
            {
                if (!rec[key])
                    flags |= flag;
                else
                    flags &= ~flag;
            }
        }

        void assignColor(ESM::MagicEffect& effect, const Misc::Color& color)
        {
            effect.mData.mRed = std::clamp(static_cast<int32_t>(color.r() * 255), 0, 255);
            effect.mData.mGreen = std::clamp(static_cast<int32_t>(color.g() * 255), 0, 255);
            effect.mData.mBlue = std::clamp(static_cast<int32_t>(color.b() * 255), 0, 255);
        }

        template <class T>
        void addMagicEffectType(sol::state_view& lua, std::string_view name)
        {
            sol::usertype<T> record = lua.new_usertype<T>(name);

            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_MagicEffect[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });

            Types::addIconProperty(record);
            Types::addProperty(record, "particle", &ESM::MagicEffect::mParticle);
            Types::addFlagProperty(record, "continuousVfx", ESM::MagicEffect::ContinuousVfx, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addProperty(record, "areaSound", &ESM::MagicEffect::mAreaSound);
            Types::addProperty(record, "boltSound", &ESM::MagicEffect::mBoltSound);
            Types::addProperty(record, "castSound", &ESM::MagicEffect::mCastSound);
            Types::addProperty(record, "hitSound", &ESM::MagicEffect::mHitSound);
            Types::addProperty(record, "areaStatic", &ESM::MagicEffect::mArea);
            Types::addProperty(record, "bolt", &ESM::MagicEffect::mBolt);
            Types::addProperty(record, "castStatic", &ESM::MagicEffect::mCasting);
            Types::addProperty(record, "hitStatic", &ESM::MagicEffect::mHit);
            Types::addProperty(record, "name", &ESM::MagicEffect::mName);
            Types::addProperty(record, "school", &ESM::MagicEffect::mData, &ESM::MagicEffect::MEDTstruct::mSchool);
            Types::addProperty(record, "baseCost", &ESM::MagicEffect::mData, &ESM::MagicEffect::MEDTstruct::mBaseCost);
            const auto getColor = [](const T& rec) -> Misc::Color {
                const ESM::MagicEffect& effect = Types::RecordType<T>::asRecord(rec);
                return Misc::Color(
                    effect.mData.mRed / 255.f, effect.mData.mGreen / 255.f, effect.mData.mBlue / 255.f, 1.f);
            };
            if constexpr (Types::RecordType<T>::isMutable)
            {
                record["color"] = sol::property(std::move(getColor), [](T& rec, const Misc::Color& color) {
                    ESM::MagicEffect& effect = rec.find();
                    assignColor(effect, color);
                });
            }
            else
            {
                record["color"] = sol::readonly_property(std::move(getColor));
            }
            Types::addReverseFlagProperty(record, "hasDuration", ESM::MagicEffect::NoDuration, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addReverseFlagProperty(record, "hasMagnitude", ESM::MagicEffect::NoMagnitude,
                &ESM::MagicEffect::mData, &ESM::MagicEffect::MEDTstruct::mFlags);
            // TODO: Not self-explanatory. Needs either a better name or documentation. The description in
            // loadmgef.hpp is uninformative.
            Types::addFlagProperty(record, "isAppliedOnce", ESM::MagicEffect::AppliedOnce, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "harmful", ESM::MagicEffect::Harmful, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "casterLinked", ESM::MagicEffect::CasterLinked, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "nonRecastable", ESM::MagicEffect::NonRecastable, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);

            Types::addFlagProperty(record, "hasAttribute", ESM::MagicEffect::TargetAttribute, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "hasSkill", ESM::MagicEffect::TargetSkill, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "onSelf", ESM::MagicEffect::CastSelf, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "onTouch", ESM::MagicEffect::CastTouch, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "onTarget", ESM::MagicEffect::CastTarget, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "unreflectable", ESM::MagicEffect::Unreflectable, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "allowsSpellmaking", ESM::MagicEffect::AllowSpellmaking,
                &ESM::MagicEffect::mData, &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "allowsEnchanting", ESM::MagicEffect::AllowEnchanting,
                &ESM::MagicEffect::mData, &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addFlagProperty(record, "negativeLight", ESM::MagicEffect::NegativeLight, &ESM::MagicEffect::mData,
                &ESM::MagicEffect::MEDTstruct::mFlags);
            Types::addProperty(record, "speed", &ESM::MagicEffect::mData, &ESM::MagicEffect::MEDTstruct::mSpeed);
        }
    }

    void addSpellBindings(sol::state_view& state)
    {
        addSpellType<ESM::Spell>(state, "ESM3_Spell");
    }

    void addMutableSpellType(sol::state_view& lua)
    {
        addSpellType<MutableRecord<ESM::Spell>>(lua, "ESM3_MutableSpell");
    }

    ESM::Spell tableToSpell(const sol::table& rec)
    {
        auto spell = Types::initFromTemplate<ESM::Spell>(rec);
        if (rec["name"] != sol::nil)
            spell.mName = rec["name"];
        if (rec["type"] != sol::nil)
            spell.mData.mType = rec["type"];
        if (spell.mData.mType < ESM::Spell::ST_Spell || spell.mData.mType > ESM::Spell::ST_Power)
            throw std::runtime_error("Invalid spell type");
        if (rec["cost"] != sol::nil)
            spell.mData.mCost = rec["cost"];
        setFlagProperty(rec, "alwaysSucceedFlag", spell.mData.mFlags, ESM::Spell::F_Always);
        setFlagProperty(rec, "starterSpellFlag", spell.mData.mFlags, ESM::Spell::F_PCStart);
        setFlagProperty(rec, "isAutocalc", spell.mData.mFlags, ESM::Spell::F_Autocalc);
        if (rec["effects"] != sol::nil)
            spell.mEffects = tableToEffectList(rec["effects"]);

        return spell;
    }

    void addEnchantmentBindings(sol::state_view& state)
    {
        addEnchantmentType<ESM::Enchantment>(state, "ESM3_Enchantment");
    }

    void addMutableEnchantmentType(sol::state_view& lua)
    {
        addEnchantmentType<MutableRecord<ESM::Enchantment>>(lua, "ESM3_MutableEnchantment");
    }

    ESM::Enchantment tableToEnchantment(const sol::table& rec)
    {
        auto enchantment = Types::initFromTemplate<ESM::Enchantment>(rec);
        if (rec["type"] != sol::nil)
            enchantment.mData.mType = rec["type"];
        if (enchantment.mData.mType < ESM::Enchantment::CastOnce
            || enchantment.mData.mType > ESM::Enchantment::ConstantEffect)
            throw std::runtime_error("Invalid enchantment type");
        if (rec["cost"] != sol::nil)
            enchantment.mData.mCost = rec["cost"];
        if (rec["charge"] != sol::nil)
            enchantment.mData.mCharge = rec["charge"];
        setFlagProperty(rec, "isAutocalc", enchantment.mData.mFlags, ESM::Enchantment::Autocalc);
        if (rec["effects"] != sol::nil)
            enchantment.mEffects = tableToEffectList(rec["effects"]);

        return enchantment;
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

    void addMagicEffectType(sol::state_view& lua)
    {
        addMagicEffectType<ESM::MagicEffect>(lua, "ESM3_MagicEffect");
    }

    void addMutableMagicEffectType(sol::state_view& lua)
    {
        addMagicEffectType<MutableRecord<ESM::MagicEffect>>(lua, "ESM3_MutableMagicEffect");
    }

    ESM::MagicEffect tableToMagicEffect(const sol::table& rec)
    {
        auto effect = Types::initFromTemplate<ESM::MagicEffect>(rec);
        if (rec["icon"] != sol::nil)
            effect.mIcon = rec["icon"];
        if (rec["particle"] != sol::nil)
            effect.mParticle = rec["particle"];
        setFlagProperty(rec, "continuousVfx", effect.mData.mFlags, ESM::MagicEffect::ContinuousVfx);
        addPropertyFromTable(rec, "areaSound", effect.mAreaSound);
        addPropertyFromTable(rec, "boltSound", effect.mBoltSound);
        addPropertyFromTable(rec, "castSound", effect.mCastSound);
        addPropertyFromTable(rec, "hitSound", effect.mHitSound);
        addPropertyFromTable(rec, "areaStatic", effect.mArea);
        addPropertyFromTable(rec, "bolt", effect.mBolt);
        addPropertyFromTable(rec, "castStatic", effect.mCasting);
        addPropertyFromTable(rec, "hitStatic", effect.mHit);
        if (rec["name"] != sol::nil)
            effect.mName = rec["name"];
        addPropertyFromTable(rec, "school", effect.mData.mSchool);
        if (rec["baseCost"] != sol::nil)
            effect.mData.mBaseCost = rec["baseCost"];
        if (rec["color"] != sol::nil)
        {
            const auto color = LuaUtil::cast<Misc::Color>(rec["color"]);
            assignColor(effect, color);
        }
        setReverseFlagProperty(rec, "hasDuration", effect.mData.mFlags, ESM::MagicEffect::NoDuration);
        setReverseFlagProperty(rec, "hasMagnitude", effect.mData.mFlags, ESM::MagicEffect::NoMagnitude);
        setFlagProperty(rec, "isAppliedOnce", effect.mData.mFlags, ESM::MagicEffect::AppliedOnce);
        setFlagProperty(rec, "harmful", effect.mData.mFlags, ESM::MagicEffect::Harmful);
        setFlagProperty(rec, "casterLinked", effect.mData.mFlags, ESM::MagicEffect::CasterLinked);
        setFlagProperty(rec, "nonRecastable", effect.mData.mFlags, ESM::MagicEffect::NonRecastable);
        setFlagProperty(rec, "hasAttribute", effect.mData.mFlags, ESM::MagicEffect::TargetAttribute);
        setFlagProperty(rec, "hasSkill", effect.mData.mFlags, ESM::MagicEffect::TargetSkill);
        setFlagProperty(rec, "onSelf", effect.mData.mFlags, ESM::MagicEffect::CastSelf);
        setFlagProperty(rec, "onTouch", effect.mData.mFlags, ESM::MagicEffect::CastTouch);
        setFlagProperty(rec, "onTarget", effect.mData.mFlags, ESM::MagicEffect::CastTarget);
        setFlagProperty(rec, "unreflectable", effect.mData.mFlags, ESM::MagicEffect::Unreflectable);
        setFlagProperty(rec, "allowsSpellmaking", effect.mData.mFlags, ESM::MagicEffect::AllowSpellmaking);
        setFlagProperty(rec, "allowsEnchanting", effect.mData.mFlags, ESM::MagicEffect::AllowEnchanting);
        setFlagProperty(rec, "negativeLight", effect.mData.mFlags, ESM::MagicEffect::NegativeLight);
        if (rec["speed"] != sol::nil)
            effect.mData.mSpeed = rec["speed"];
        return effect;
    }
}
