#include "magicbindings.hpp"

#include <components/esm3/activespells.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/color.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwmechanics/activespells.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/magiceffects.hpp"
#include "../mwmechanics/spellutil.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/worldmodel.hpp"

#include "localscripts.hpp"
#include "luamanagerimp.hpp"
#include "object.hpp"
#include "objectvariant.hpp"

namespace MWLua
{
    template <typename Store>
    struct ActorStore
    {
        using Collection = typename Store::Collection;
        using Iterator = typename Collection::const_iterator;

        ActorStore(const sol::object& actor)
            : mActor(actor)
            , mIterator()
            , mIndex(0)
        {
            if (!isActor())
                throw std::runtime_error("Actor expected");
            reset();
        }

        bool isActor() const { return !mActor.ptr().isEmpty() && mActor.ptr().getClass().isActor(); }

        bool isLObject() const { return mActor.isLObject(); }

        void reset()
        {
            mIndex = 0;
            auto* store = getStore();
            if (store)
                mIterator = store->begin();
        }

        bool isEnd() const
        {
            auto* store = getStore();
            if (store)
                return mIterator == store->end();
            return true;
        }

        void advance()
        {
            auto* store = getStore();
            if (store)
            {
                mIterator++;
                mIndex++;
            }
        }

        Store* getStore() const;

        ObjectVariant mActor;
        Iterator mIterator;
        int mIndex;
    };

    template <>
    MWMechanics::Spells* ActorStore<MWMechanics::Spells>::getStore() const
    {
        if (!isActor())
            return nullptr;
        const MWWorld::Ptr& ptr = mActor.ptr();
        return &ptr.getClass().getCreatureStats(ptr).getSpells();
    }

    template <>
    MWMechanics::MagicEffects* ActorStore<MWMechanics::MagicEffects>::getStore() const
    {
        if (!isActor())
            return nullptr;
        const MWWorld::Ptr& ptr = mActor.ptr();
        return &ptr.getClass().getCreatureStats(ptr).getMagicEffects();
    }

    template <>
    MWMechanics::ActiveSpells* ActorStore<MWMechanics::ActiveSpells>::getStore() const
    {
        if (!isActor())
            return nullptr;
        const MWWorld::Ptr& ptr = mActor.ptr();
        return &ptr.getClass().getCreatureStats(ptr).getActiveSpells();
    }

    struct ActiveEffect
    {
        MWMechanics::EffectKey key;
        MWMechanics::EffectParam param;
    };
    struct ActiveSpell
    {
        ObjectVariant mActor;
        MWMechanics::ActiveSpells::ActiveSpellParams mParams;
    };

    // class returned via 'types.Actor.spells(obj)' in Lua
    using ActorSpells = ActorStore<MWMechanics::Spells>;
    // class returned via 'types.Actor.activeEffects(obj)' in Lua
    using ActorActiveEffects = ActorStore<MWMechanics::MagicEffects>;
    // class returned via 'types.Actor.activeSpells(obj)' in Lua
    using ActorActiveSpells = ActorStore<MWMechanics::ActiveSpells>;
}

namespace sol
{
    template <typename T>
    struct is_automagical<typename MWWorld::Store<T>> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::Spell> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::ENAMstruct> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::MagicEffect> : std::false_type
    {
    };
    template <typename T>
    struct is_automagical<MWLua::ActorStore<T>> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::ActiveSpell> : std::false_type
    {
    };
}

namespace MWLua
{
    static ESM::RefId toSpellId(const sol::object& spellOrId)
    {
        if (spellOrId.is<ESM::Spell>())
            return spellOrId.as<const ESM::Spell*>()->mId;
        else
            return ESM::RefId::deserializeText(LuaUtil::cast<std::string_view>(spellOrId));
    }
    static ESM::RefId toRecordId(const sol::object& recordOrId)
    {
        if (recordOrId.is<ESM::Spell>())
            return recordOrId.as<const ESM::Spell*>()->mId;
        else if (recordOrId.is<ESM::Potion>())
            return recordOrId.as<const ESM::Potion*>()->mId;
        else if (recordOrId.is<ESM::Ingredient>())
            return recordOrId.as<const ESM::Ingredient*>()->mId;
        else if (recordOrId.is<ESM::Enchantment>())
            return recordOrId.as<const ESM::Enchantment*>()->mId;
        else if (recordOrId.is<ESM::Armor>())
            return recordOrId.as<const ESM::Armor*>()->mId;
        else if (recordOrId.is<ESM::Book>())
            return recordOrId.as<const ESM::Book*>()->mId;
        else if (recordOrId.is<ESM::Clothing>())
            return recordOrId.as<const ESM::Clothing*>()->mId;
        else if (recordOrId.is<ESM::Weapon>())
            return recordOrId.as<const ESM::Weapon*>()->mId;
        else
            return ESM::RefId::deserializeText(LuaUtil::cast<std::string_view>(recordOrId));
    }

    sol::table initCoreMagicBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::table magicApi(lua, sol::create);

        // Constants
        magicApi["RANGE"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, ESM::RangeType>({
            { "Self", ESM::RT_Self },
            { "Touch", ESM::RT_Touch },
            { "Target", ESM::RT_Target },
        }));
        magicApi["SPELL_TYPE"]
            = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, ESM::Spell::SpellType>({
                { "Spell", ESM::Spell::ST_Spell },
                { "Ability", ESM::Spell::ST_Ability },
                { "Blight", ESM::Spell::ST_Blight },
                { "Disease", ESM::Spell::ST_Disease },
                { "Curse", ESM::Spell::ST_Curse },
                { "Power", ESM::Spell::ST_Power },
            }));
        magicApi["ENCHANTMENT_TYPE"]
            = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, ESM::Enchantment::Type>({
                { "CastOnce", ESM::Enchantment::Type::CastOnce },
                { "CastOnStrike", ESM::Enchantment::Type::WhenStrikes },
                { "CastOnUse", ESM::Enchantment::Type::WhenUsed },
                { "ConstantEffect", ESM::Enchantment::Type::ConstantEffect },
            }));

        sol::table effect(context.mLua->sol(), sol::create);
        magicApi["EFFECT_TYPE"] = LuaUtil::makeStrictReadOnly(effect);
        for (const auto& name : ESM::MagicEffect::sIndexNames)
        {
            effect[name] = Misc::StringUtils::lowerCase(name);
        }

        // Spell store
        using SpellStore = MWWorld::Store<ESM::Spell>;
        const SpellStore* spellStore = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();
        sol::usertype<SpellStore> spellStoreT = lua.new_usertype<SpellStore>("ESM3_SpellStore");
        spellStoreT[sol::meta_function::to_string]
            = [](const SpellStore& store) { return "ESM3_SpellStore{" + std::to_string(store.getSize()) + " spells}"; };
        spellStoreT[sol::meta_function::length] = [](const SpellStore& store) { return store.getSize(); };
        spellStoreT[sol::meta_function::index] = sol::overload(
            [](const SpellStore& store, size_t index) -> const ESM::Spell* {
                if (index == 0 || index > store.getSize())
                    return nullptr;
                return store.at(index - 1);
            },
            [](const SpellStore& store, std::string_view spellId) -> const ESM::Spell* {
                return store.search(ESM::RefId::deserializeText(spellId));
            });
        spellStoreT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        spellStoreT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();

        magicApi["spells"] = spellStore;

        // Enchantment store
        using EnchantmentStore = MWWorld::Store<ESM::Enchantment>;
        const EnchantmentStore* enchantmentStore
            = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>();
        sol::usertype<EnchantmentStore> enchantmentStoreT = lua.new_usertype<EnchantmentStore>("ESM3_EnchantmentStore");
        enchantmentStoreT[sol::meta_function::to_string] = [](const EnchantmentStore& store) {
            return "ESM3_EnchantmentStore{" + std::to_string(store.getSize()) + " enchantments}";
        };
        enchantmentStoreT[sol::meta_function::length] = [](const EnchantmentStore& store) { return store.getSize(); };
        enchantmentStoreT[sol::meta_function::index] = sol::overload(
            [](const EnchantmentStore& store, size_t index) -> const ESM::Enchantment* {
                if (index == 0 || index > store.getSize())
                    return nullptr;
                return store.at(index - 1);
            },
            [](const EnchantmentStore& store, std::string_view enchantmentId) -> const ESM::Enchantment* {
                return store.search(ESM::RefId::deserializeText(enchantmentId));
            });
        enchantmentStoreT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        enchantmentStoreT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();

        magicApi["enchantments"] = enchantmentStore;

        // MagicEffect store
        using MagicEffectStore = MWWorld::Store<ESM::MagicEffect>;
        const MagicEffectStore* magicEffectStore
            = &MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>();
        auto magicEffectStoreT = lua.new_usertype<MagicEffectStore>("ESM3_MagicEffectStore");
        magicEffectStoreT[sol::meta_function::to_string] = [](const MagicEffectStore& store) {
            return "ESM3_MagicEffectStore{" + std::to_string(store.getSize()) + " effects}";
        };
        magicEffectStoreT[sol::meta_function::index] = sol::overload(
            [](const MagicEffectStore& store, int id) -> const ESM::MagicEffect* { return store.search(id); },
            [](const MagicEffectStore& store, std::string_view id) -> const ESM::MagicEffect* {
                int index = ESM::MagicEffect::indexNameToIndex(id);
                return store.search(index);
            });
        auto magicEffectsIter = [magicEffectStore](sol::this_state lua, const sol::object& /*store*/,
                                    sol::optional<int> id) -> std::tuple<sol::object, sol::object> {
            MagicEffectStore::iterator iter;
            if (id.has_value())
            {
                iter = magicEffectStore->findIter(*id);
                if (iter != magicEffectStore->end())
                    iter++;
            }
            else
                iter = magicEffectStore->begin();
            if (iter != magicEffectStore->end())
                return std::make_tuple(sol::make_object(lua, iter->first), sol::make_object(lua, &iter->second));
            else
                return std::make_tuple(sol::nil, sol::nil);
        };
        magicEffectStoreT[sol::meta_function::pairs]
            = [iter = sol::make_object(lua, magicEffectsIter)] { return iter; };

        magicApi["effects"] = magicEffectStore;

        // Spell record
        auto spellT = lua.new_usertype<ESM::Spell>("ESM3_Spell");
        spellT[sol::meta_function::to_string]
            = [](const ESM::Spell& rec) -> std::string { return "ESM3_Spell[" + rec.mId.toDebugString() + "]"; };
        spellT["id"] = sol::readonly_property([](const ESM::Spell& rec) { return rec.mId.serializeText(); });
        spellT["name"] = sol::readonly_property([](const ESM::Spell& rec) -> std::string_view { return rec.mName; });
        spellT["type"] = sol::readonly_property([](const ESM::Spell& rec) -> int { return rec.mData.mType; });
        spellT["cost"] = sol::readonly_property([](const ESM::Spell& rec) -> int { return rec.mData.mCost; });
        spellT["effects"] = sol::readonly_property([&lua](const ESM::Spell& rec) -> sol::table {
            sol::table res(lua, sol::create);
            for (size_t i = 0; i < rec.mEffects.mList.size(); ++i)
                res[i + 1] = rec.mEffects.mList[i]; // ESM::ENAMstruct (effect params)
            return res;
        });

        // Enchantment record
        auto enchantT = lua.new_usertype<ESM::Enchantment>("ESM3_Enchantment");
        enchantT[sol::meta_function::to_string] = [](const ESM::Enchantment& rec) -> std::string {
            return "ESM3_Enchantment[" + rec.mId.toDebugString() + "]";
        };
        enchantT["id"] = sol::readonly_property([](const ESM::Enchantment& rec) { return rec.mId.serializeText(); });
        enchantT["type"] = sol::readonly_property([](const ESM::Enchantment& rec) -> int { return rec.mData.mType; });
        enchantT["autocalcFlag"] = sol::readonly_property(
            [](const ESM::Enchantment& rec) -> bool { return !!(rec.mData.mFlags & ESM::Enchantment::Autocalc); });
        enchantT["cost"] = sol::readonly_property([](const ESM::Enchantment& rec) -> int { return rec.mData.mCost; });
        enchantT["charge"]
            = sol::readonly_property([](const ESM::Enchantment& rec) -> int { return rec.mData.mCharge; });
        enchantT["effects"] = sol::readonly_property([&lua](const ESM::Enchantment& rec) -> sol::table {
            sol::table res(lua, sol::create);
            for (size_t i = 0; i < rec.mEffects.mList.size(); ++i)
                res[i + 1] = rec.mEffects.mList[i]; // ESM::ENAMstruct (effect params)
            return res;
        });

        // Effect params
        auto effectParamsT = lua.new_usertype<ESM::ENAMstruct>("ESM3_EffectParams");
        effectParamsT[sol::meta_function::to_string] = [magicEffectStore](const ESM::ENAMstruct& params) {
            const ESM::MagicEffect* const rec = magicEffectStore->find(params.mEffectID);
            return "ESM3_EffectParams[" + ESM::MagicEffect::indexToGmstString(rec->mIndex) + "]";
        };
        effectParamsT["effect"]
            = sol::readonly_property([magicEffectStore](const ESM::ENAMstruct& params) -> const ESM::MagicEffect* {
                  return magicEffectStore->find(params.mEffectID);
              });
        effectParamsT["affectedSkill"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> sol::optional<std::string> {
                  ESM::RefId id = ESM::Skill::indexToRefId(params.mSkill);
                  if (!id.empty())
                      return id.serializeText();
                  return sol::nullopt;
              });
        effectParamsT["affectedAttribute"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> sol::optional<std::string> {
                  ESM::RefId id = ESM::Attribute::indexToRefId(params.mAttribute);
                  if (!id.empty())
                      return id.serializeText();
                  return sol::nullopt;
              });
        effectParamsT["range"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> int { return params.mRange; });
        effectParamsT["area"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> int { return params.mArea; });
        effectParamsT["magnitudeMin"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> int { return params.mMagnMin; });
        effectParamsT["magnitudeMax"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> int { return params.mMagnMax; });
        effectParamsT["duration"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> int { return params.mDuration; });

        // MagicEffect record
        auto magicEffectT = context.mLua->sol().new_usertype<ESM::MagicEffect>("ESM3_MagicEffect");

        magicEffectT[sol::meta_function::to_string] = [](const ESM::MagicEffect& rec) {
            return "ESM3_MagicEffect[" + ESM::MagicEffect::indexToGmstString(rec.mIndex) + "]";
        };
        magicEffectT["id"] = sol::readonly_property([](const ESM::MagicEffect& rec) -> std::string {
            auto name = ESM::MagicEffect::indexToName(rec.mIndex);
            return Misc::StringUtils::lowerCase(name);
        });
        magicEffectT["icon"] = sol::readonly_property([](const ESM::MagicEffect& rec) -> std::string {
            auto vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
            return Misc::ResourceHelpers::correctIconPath(rec.mIcon, vfs);
        });
        magicEffectT["particle"]
            = sol::readonly_property([](const ESM::MagicEffect& rec) -> std::string_view { return rec.mParticle; });
        magicEffectT["continuousVfx"] = sol::readonly_property([](const ESM::MagicEffect& rec) -> bool {
            return (rec.mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0;
        });
        magicEffectT["castingStatic"] = sol::readonly_property(
            [](const ESM::MagicEffect& rec) -> std::string { return rec.mCasting.serializeText(); });
        magicEffectT["hitStatic"] = sol::readonly_property(
            [](const ESM::MagicEffect& rec) -> std::string { return rec.mHit.serializeText(); });
        magicEffectT["areaStatic"] = sol::readonly_property(
            [](const ESM::MagicEffect& rec) -> std::string { return rec.mArea.serializeText(); });
        magicEffectT["name"] = sol::readonly_property([](const ESM::MagicEffect& rec) -> std::string_view {
            return MWBase::Environment::get()
                .getWorld()
                ->getStore()
                .get<ESM::GameSetting>()
                .find(ESM::MagicEffect::indexToGmstString(rec.mIndex))
                ->mValue.getString();
        });
        magicEffectT["school"] = sol::readonly_property(
            [](const ESM::MagicEffect& rec) -> std::string { return rec.mData.mSchool.serializeText(); });
        magicEffectT["baseCost"]
            = sol::readonly_property([](const ESM::MagicEffect& rec) -> float { return rec.mData.mBaseCost; });
        magicEffectT["color"] = sol::readonly_property([](const ESM::MagicEffect& rec) -> Misc::Color {
            return Misc::Color(rec.mData.mRed / 255.f, rec.mData.mGreen / 255.f, rec.mData.mBlue / 255.f, 1.f);
        });
        magicEffectT["harmful"] = sol::readonly_property(
            [](const ESM::MagicEffect& rec) -> bool { return rec.mData.mFlags & ESM::MagicEffect::Harmful; });

        // TODO: Should we expose it? What happens if a spell has several effects with different projectileSpeed?
        // magicEffectT["projectileSpeed"]
        //     = sol::readonly_property([](const ESM::MagicEffect& rec) -> float { return rec.mData.mSpeed; });

        auto activeSpellEffectT = context.mLua->sol().new_usertype<ESM::ActiveEffect>("ActiveSpellEffect");
        activeSpellEffectT[sol::meta_function::to_string] = [](const ESM::ActiveEffect& effect) {
            return "ActiveSpellEffect[" + ESM::MagicEffect::indexToGmstString(effect.mEffectId) + "]";
        };
        activeSpellEffectT["id"] = sol::readonly_property([](const ESM::ActiveEffect& effect) -> std::string {
            auto name = ESM::MagicEffect::indexToName(effect.mEffectId);
            return Misc::StringUtils::lowerCase(name);
        });
        activeSpellEffectT["name"] = sol::readonly_property([](const ESM::ActiveEffect& effect) -> std::string {
            return MWMechanics::EffectKey(effect.mEffectId, effect.getSkillOrAttribute()).toString();
        });
        activeSpellEffectT["affectedSkill"]
            = sol::readonly_property([magicEffectStore](const ESM::ActiveEffect& effect) -> sol::optional<std::string> {
                  auto* rec = magicEffectStore->find(effect.mEffectId);
                  if (rec->mData.mFlags & ESM::MagicEffect::TargetSkill)
                      return effect.getSkillOrAttribute().serializeText();
                  else
                      return sol::nullopt;
              });
        activeSpellEffectT["affectedAttribute"]
            = sol::readonly_property([magicEffectStore](const ESM::ActiveEffect& effect) -> sol::optional<std::string> {
                  auto* rec = magicEffectStore->find(effect.mEffectId);
                  if (rec->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                      return effect.getSkillOrAttribute().serializeText();
                  else
                      return sol::nullopt;
              });
        activeSpellEffectT["magnitudeThisFrame"]
            = sol::readonly_property([magicEffectStore](const ESM::ActiveEffect& effect) -> sol::optional<float> {
                  auto* rec = magicEffectStore->find(effect.mEffectId);
                  if (rec->mData.mFlags & ESM::MagicEffect::Flags::NoMagnitude)
                      return sol::nullopt;
                  return effect.mMagnitude;
              });
        activeSpellEffectT["minMagnitude"]
            = sol::readonly_property([magicEffectStore](const ESM::ActiveEffect& effect) -> sol::optional<float> {
                  auto* rec = magicEffectStore->find(effect.mEffectId);
                  if (rec->mData.mFlags & ESM::MagicEffect::Flags::NoMagnitude)
                      return sol::nullopt;
                  return effect.mMinMagnitude;
              });
        activeSpellEffectT["maxMagnitude"]
            = sol::readonly_property([magicEffectStore](const ESM::ActiveEffect& effect) -> sol::optional<float> {
                  auto* rec = magicEffectStore->find(effect.mEffectId);
                  if (rec->mData.mFlags & ESM::MagicEffect::Flags::NoMagnitude)
                      return sol::nullopt;
                  return effect.mMaxMagnitude;
              });
        activeSpellEffectT["durationLeft"]
            = sol::readonly_property([magicEffectStore](const ESM::ActiveEffect& effect) -> sol::optional<float> {
                  // Permanent/constant effects, abilities, etc. will have a negative duration
                  if (effect.mDuration < 0)
                      return sol::nullopt;
                  auto* rec = magicEffectStore->find(effect.mEffectId);
                  if (rec->mData.mFlags & ESM::MagicEffect::Flags::NoDuration)
                      return sol::nullopt;
                  return effect.mTimeLeft;
              });
        activeSpellEffectT["duration"]
            = sol::readonly_property([magicEffectStore](const ESM::ActiveEffect& effect) -> sol::optional<float> {
                  // Permanent/constant effects, abilities, etc. will have a negative duration
                  if (effect.mDuration < 0)
                      return sol::nullopt;
                  auto* rec = magicEffectStore->find(effect.mEffectId);
                  if (rec->mData.mFlags & ESM::MagicEffect::Flags::NoDuration)
                      return sol::nullopt;
                  return effect.mDuration;
              });

        auto activeSpellT = context.mLua->sol().new_usertype<ActiveSpell>("ActiveSpellParams");
        activeSpellT[sol::meta_function::to_string] = [](const ActiveSpell& activeSpell) {
            return "ActiveSpellParams[" + activeSpell.mParams.getId().serializeText() + "]";
        };
        activeSpellT["name"] = sol::readonly_property(
            [](const ActiveSpell& activeSpell) -> std::string_view { return activeSpell.mParams.getDisplayName(); });
        activeSpellT["id"] = sol::readonly_property(
            [](const ActiveSpell& activeSpell) -> std::string { return activeSpell.mParams.getId().serializeText(); });
        activeSpellT["item"] = sol::readonly_property([&lua](const ActiveSpell& activeSpell) -> sol::object {
            auto item = activeSpell.mParams.getItem();
            if (!item.isSet())
                return sol::nil;
            auto itemPtr = MWBase::Environment::get().getWorldModel()->getPtr(item);
            if (itemPtr.isEmpty())
                return sol::nil;
            if (activeSpell.mActor.isGObject())
                return sol::make_object(lua, GObject(itemPtr));
            else
                return sol::make_object(lua, LObject(itemPtr));
        });
        activeSpellT["caster"] = sol::readonly_property([&lua](const ActiveSpell& activeSpell) -> sol::object {
            auto caster
                = MWBase::Environment::get().getWorld()->searchPtrViaActorId(activeSpell.mParams.getCasterActorId());
            if (caster.isEmpty())
                return sol::nil;
            else
            {
                if (activeSpell.mActor.isGObject())
                    return sol::make_object(lua, GObject(getId(caster)));
                else
                    return sol::make_object(lua, LObject(getId(caster)));
            }
        });
        activeSpellT["effects"] = sol::readonly_property([&lua](const ActiveSpell& activeSpell) -> sol::table {
            sol::table res(lua, sol::create);
            size_t tableIndex = 0;
            for (const ESM::ActiveEffect& effect : activeSpell.mParams.getEffects())
            {
                if (!(effect.mFlags & ESM::ActiveEffect::Flag_Applied))
                    continue;
                res[++tableIndex] = effect; // ESM::ActiveEffect (effect params)
            }
            return res;
        });

        auto activeEffectT = context.mLua->sol().new_usertype<ActiveEffect>("ActiveEffect");

        activeEffectT[sol::meta_function::to_string] = [](const ActiveEffect& effect) {
            return "ActiveEffect[" + ESM::MagicEffect::indexToGmstString(effect.key.mId) + "]";
        };
        activeEffectT["id"] = sol::readonly_property([](const ActiveEffect& effect) -> std::string {
            auto name = ESM::MagicEffect::indexToName(effect.key.mId);
            return Misc::StringUtils::lowerCase(name);
        });
        activeEffectT["name"]
            = sol::readonly_property([](const ActiveEffect& effect) -> std::string { return effect.key.toString(); });

        activeEffectT["affectedSkill"]
            = sol::readonly_property([magicEffectStore](const ActiveEffect& effect) -> sol::optional<std::string> {
                  auto* rec = magicEffectStore->find(effect.key.mId);
                  if (rec->mData.mFlags & ESM::MagicEffect::TargetSkill)
                      return effect.key.mArg.serializeText();
                  return sol::nullopt;
              });
        activeEffectT["affectedAttribute"]
            = sol::readonly_property([magicEffectStore](const ActiveEffect& effect) -> sol::optional<std::string> {
                  auto* rec = magicEffectStore->find(effect.key.mId);
                  if (rec->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                      return effect.key.mArg.serializeText();
                  return sol::nullopt;
              });

        activeEffectT["magnitude"]
            = sol::readonly_property([](const ActiveEffect& effect) { return effect.param.getMagnitude(); });
        activeEffectT["magnitudeBase"]
            = sol::readonly_property([](const ActiveEffect& effect) { return effect.param.getBase(); });
        activeEffectT["magnitudeModifier"]
            = sol::readonly_property([](const ActiveEffect& effect) { return effect.param.getModifier(); });

        return LuaUtil::makeReadOnly(magicApi);
    }

    void addActorMagicBindings(sol::table& actor, const Context& context)
    {
        const MWWorld::Store<ESM::Spell>* spellStore
            = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();

        // types.Actor.spells(o)
        actor["spells"] = [](const sol::object& actor) { return ActorSpells{ actor }; };
        auto spellsT = context.mLua->sol().new_usertype<ActorSpells>("ActorSpells");
        spellsT[sol::meta_function::to_string]
            = [](const ActorSpells& spells) { return "ActorSpells[" + spells.mActor.object().toString() + "]"; };

        actor["activeSpells"] = [](const sol::object& actor) { return ActorActiveSpells{ actor }; };
        auto activeSpellsT = context.mLua->sol().new_usertype<ActorActiveSpells>("ActorActiveSpells");
        activeSpellsT[sol::meta_function::to_string] = [](const ActorActiveSpells& spells) {
            return "ActorActiveSpells[" + spells.mActor.object().toString() + "]";
        };

        actor["activeEffects"] = [](const sol::object& actor) { return ActorActiveEffects{ actor }; };
        auto activeEffectsT = context.mLua->sol().new_usertype<ActorActiveEffects>("ActorActiveEffects");
        activeEffectsT[sol::meta_function::to_string] = [](const ActorActiveEffects& effects) {
            return "ActorActiveEffects[" + effects.mActor.object().toString() + "]";
        };

        actor["getSelectedSpell"] = [spellStore](const Object& o) -> sol::optional<const ESM::Spell*> {
            const MWWorld::Ptr& ptr = o.ptr();
            const MWWorld::Class& cls = ptr.getClass();
            if (!cls.isActor())
                throw std::runtime_error("Actor expected");
            ESM::RefId spellId;
            if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                spellId = MWBase::Environment::get().getWindowManager()->getSelectedSpell();
            else
                spellId = cls.getCreatureStats(ptr).getSpells().getSelectedSpell();
            if (spellId.empty())
                return sol::nullopt;
            else
                return spellStore->find(spellId);
        };
        actor["setSelectedSpell"] = [context, spellStore](const SelfObject& o, const sol::object& spellOrId) {
            const MWWorld::Ptr& ptr = o.ptr();
            const MWWorld::Class& cls = ptr.getClass();
            if (!cls.isActor())
                throw std::runtime_error("Actor expected");
            ESM::RefId spellId;
            if (spellOrId != sol::nil)
            {
                spellId = toSpellId(spellOrId);
                const ESM::Spell* spell = spellStore->find(spellId);
                if (spell->mData.mType != ESM::Spell::ST_Spell && spell->mData.mType != ESM::Spell::ST_Power)
                    throw std::runtime_error("Ability or disease can not be casted: " + spellId.toDebugString());
            }
            context.mLuaManager->addAction([obj = Object(ptr), spellId]() {
                const MWWorld::Ptr& ptr = obj.ptr();
                auto& stats = ptr.getClass().getCreatureStats(ptr);

                if (spellId.empty())
                {
                    if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                        MWBase::Environment::get().getWindowManager()->unsetSelectedSpell();
                    else
                        stats.getSpells().setSelectedSpell(ESM::RefId());
                    return;
                }
                if (!stats.getSpells().hasSpell(spellId))
                    throw std::runtime_error("Actor doesn't know spell " + spellId.toDebugString());

                if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                {
                    int chance = MWMechanics::getSpellSuccessChance(spellId, ptr);
                    MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, chance);
                }
                else
                    stats.getSpells().setSelectedSpell(spellId);
            });
        };

        actor["clearSelectedCastable"] = [context](const SelfObject& o) {
            if (!o.ptr().getClass().isActor())
                throw std::runtime_error("Actor expected");
            context.mLuaManager->addAction([obj = Object(o.ptr())]() {
                const MWWorld::Ptr& ptr = obj.ptr();
                auto& stats = ptr.getClass().getCreatureStats(ptr);
                if (ptr.getClass().hasInventoryStore(ptr))
                {
                    MWWorld::InventoryStore& inventory = ptr.getClass().getInventoryStore(ptr);
                    inventory.setSelectedEnchantItem(inventory.end());
                }
                if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                    MWBase::Environment::get().getWindowManager()->unsetSelectedSpell();
                else
                    stats.getSpells().setSelectedSpell(ESM::RefId());
            });
        };

        // #(types.Actor.spells(o))
        spellsT[sol::meta_function::length] = [](const ActorSpells& spells) -> size_t {
            if (auto* store = spells.getStore())
                return store->count();
            return 0;
        };

        // types.Actor.spells(o)[i]
        spellsT[sol::meta_function::index] = sol::overload(
            [](const ActorSpells& spells, size_t index) -> const ESM::Spell* {
                if (auto* store = spells.getStore())
                    if (index <= store->count() && index > 0)
                        return store->at(index - 1);
                return nullptr;
            },
            [spellStore](const ActorSpells& spells, std::string_view spellId) -> const ESM::Spell* {
                if (auto* store = spells.getStore())
                {
                    const ESM::Spell* spell = spellStore->search(ESM::RefId::deserializeText(spellId));
                    if (spell && store->hasSpell(spell))
                        return spell;
                }
                return nullptr;
            });

        // pairs(types.Actor.spells(o))
        spellsT[sol::meta_function::pairs] = context.mLua->sol()["ipairsForArray"].template get<sol::function>();

        // ipairs(types.Actor.spells(o))
        spellsT[sol::meta_function::ipairs] = context.mLua->sol()["ipairsForArray"].template get<sol::function>();

        // types.Actor.spells(o):add(id)
        spellsT["add"] = [context](const ActorSpells& spells, const sol::object& spellOrId) {
            if (spells.mActor.isLObject())
                throw std::runtime_error("Local scripts can modify only spells of the actor they are attached to.");
            context.mLuaManager->addAction([obj = spells.mActor.object(), id = toSpellId(spellOrId)]() {
                const MWWorld::Ptr& ptr = obj.ptr();
                if (ptr.getClass().isActor())
                    ptr.getClass().getCreatureStats(ptr).getSpells().add(id);
            });
        };

        // types.Actor.spells(o):remove(id)
        spellsT["remove"] = [context](const ActorSpells& spells, const sol::object& spellOrId) {
            if (spells.mActor.isLObject())
                throw std::runtime_error("Local scripts can modify only spells of the actor they are attached to.");
            context.mLuaManager->addAction([obj = spells.mActor.object(), id = toSpellId(spellOrId)]() {
                const MWWorld::Ptr& ptr = obj.ptr();
                if (ptr.getClass().isActor())
                    ptr.getClass().getCreatureStats(ptr).getSpells().remove(id);
            });
        };

        // types.Actor.spells(o):clear()
        spellsT["clear"] = [context](const ActorSpells& spells) {
            if (spells.mActor.isLObject())
                throw std::runtime_error("Local scripts can modify only spells of the actor they are attached to.");
            context.mLuaManager->addAction([obj = spells.mActor.object()]() {
                const MWWorld::Ptr& ptr = obj.ptr();
                if (ptr.getClass().isActor())
                    ptr.getClass().getCreatureStats(ptr).getSpells().clear();
            });
        };

        // pairs(types.Actor.activeSpells(o))
        activeSpellsT["__pairs"] = [](sol::this_state ts, ActorActiveSpells& self) {
            sol::state_view lua(ts);
            self.reset();
            return sol::as_function([lua, self]() mutable -> std::pair<sol::object, sol::object> {
                if (!self.isEnd())
                {
                    auto id = sol::make_object(lua, self.mIterator->getId().serializeText());
                    auto params = sol::make_object(lua, ActiveSpell{ self.mActor, *self.mIterator });
                    self.advance();
                    return { id, params };
                }
                else
                {
                    return { sol::lua_nil, sol::lua_nil };
                }
            });
        };

        // types.Actor.activeSpells(o):isSpellActive(id)
        activeSpellsT["isSpellActive"]
            = [](const ActorActiveSpells& activeSpells, const sol::object& recordOrId) -> bool {
            if (auto* store = activeSpells.getStore())
            {
                auto id = toRecordId(recordOrId);
                return store->isSpellActive(id) || store->isEnchantmentActive(id);
            }
            return false;
        };

        // types.Actor.activeSpells(o):remove(id)
        activeSpellsT["remove"] = [](const ActorActiveSpells& spells, const sol::object& spellOrId) {
            if (spells.isLObject())
                throw std::runtime_error("Local scripts can modify effect only on the actor they are attached to.");

            auto id = toSpellId(spellOrId);
            if (auto* store = spells.getStore())
            {
                store->removeEffects(spells.mActor.ptr(), id);
            }
        };

        // pairs(types.Actor.activeEffects(o))
        // Note that the indexes are fake, and only for consistency with other lua pairs interfaces. You can't use them
        // for anything.
        activeEffectsT["__pairs"] = [](sol::this_state ts, ActorActiveEffects& self) {
            sol::state_view lua(ts);
            self.reset();
            return sol::as_function([lua, self]() mutable -> std::pair<sol::object, sol::object> {
                while (!self.isEnd())
                {
                    if (self.mIterator->second.getBase() == 0 && self.mIterator->second.getModifier() == 0.f)
                    {
                        self.advance();
                        continue;
                    }
                    ActiveEffect effect = ActiveEffect{ self.mIterator->first, self.mIterator->second };
                    auto result = sol::make_object(lua, effect);

                    auto key = sol::make_object(lua, self.mIterator->first.toString());
                    self.advance();
                    return { key, result };
                }
                return { sol::lua_nil, sol::lua_nil };
            });
        };

        auto getEffectKey
            = [](std::string_view idStr, sol::optional<std::string_view> argStr) -> MWMechanics::EffectKey {
            auto id = ESM::MagicEffect::indexNameToIndex(idStr);
            auto* rec = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(id);

            MWMechanics::EffectKey key = MWMechanics::EffectKey(id);

            if (argStr.has_value()
                && (rec->mData.mFlags & (ESM::MagicEffect::TargetAttribute | ESM::MagicEffect::TargetSkill)))
            {
                // MWLua exposes attributes and skills as strings, so we have to convert them back to IDs here
                if (rec->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                {
                    ESM::RefId attribute = ESM::RefId::deserializeText(argStr.value());
                    key = MWMechanics::EffectKey(id, attribute);
                }

                if (rec->mData.mFlags & ESM::MagicEffect::TargetSkill)
                {
                    ESM::RefId skill = ESM::RefId::deserializeText(argStr.value());
                    key = MWMechanics::EffectKey(id, skill);
                }
            }

            return key;
        };

        // types.Actor.activeEffects(o):getEffect(id, ?arg)
        activeEffectsT["getEffect"] = [getEffectKey](const ActorActiveEffects& effects, std::string_view idStr,
                                          sol::optional<std::string_view> argStr) -> sol::optional<ActiveEffect> {
            if (!effects.isActor())
                return sol::nullopt;

            MWMechanics::EffectKey key = getEffectKey(idStr, argStr);

            if (auto* store = effects.getStore())
                if (auto effect = store->get(key))
                    return ActiveEffect{ key, effect.value() };
            return ActiveEffect{ key, MWMechanics::EffectParam() };
        };

        // types.Actor.activeEffects(o):removeEffect(id, ?arg)
        activeEffectsT["remove"] = [getEffectKey](const ActorActiveEffects& effects, std::string_view idStr,
                                       sol::optional<std::string_view> argStr) {
            if (!effects.isActor())
                return;

            if (effects.isLObject())
                throw std::runtime_error("Local scripts can modify effect only on the actor they are attached to.");

            MWMechanics::EffectKey key = getEffectKey(idStr, argStr);

            // Note that, although this is member method of ActorActiveEffects and we are removing an effect (not a
            // spell), we still need to use the active spells store to purge this effect from active spells.
            const auto& ptr = effects.mActor.ptr();

            auto& activeSpells = ptr.getClass().getCreatureStats(ptr).getActiveSpells();
            activeSpells.purgeEffect(ptr, key.mId, key.mArg);
        };

        // types.Actor.activeEffects(o):set(value, id, ?arg)
        activeEffectsT["set"] = [getEffectKey](const ActorActiveEffects& effects, int value, std::string_view idStr,
                                    sol::optional<std::string_view> argStr) {
            if (!effects.isActor())
                return;

            if (effects.isLObject())
                throw std::runtime_error("Local scripts can modify effect only on the actor they are attached to.");

            MWMechanics::EffectKey key = getEffectKey(idStr, argStr);
            int currentValue = effects.getStore()->getOrDefault(key).getMagnitude();
            effects.getStore()->modifyBase(key, value - currentValue);
        };

        // types.Actor.activeEffects(o):modify(value, id, ?arg)
        activeEffectsT["modify"] = [getEffectKey](const ActorActiveEffects& effects, int value, std::string_view idStr,
                                       sol::optional<std::string_view> argStr) {
            if (!effects.isActor())
                return;

            if (effects.isLObject())
                throw std::runtime_error("Local scripts can modify effect only on the actor they are attached to.");

            MWMechanics::EffectKey key = getEffectKey(idStr, argStr);
            effects.getStore()->modifyBase(key, value);
        };
    }
}
