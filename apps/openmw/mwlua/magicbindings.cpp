#include "magicbindings.hpp"

#include <components/esm3/activespells.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/color.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwmechanics/activespells.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "localscripts.hpp"
#include "luamanagerimp.hpp"
#include "object.hpp"
#include "objectvariant.hpp"
#include "worldview.hpp"

namespace MWLua
{
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
        magicApi["SCHOOL"] = LuaUtil::makeStrictReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            { "Alteration", 0 },
            { "Conjuration", 1 },
            { "Destruction", 2 },
            { "Illusion", 3 },
            { "Mysticism", 4 },
            { "Restoration", 5 },
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

        // Spell store
        using SpellStore = MWWorld::Store<ESM::Spell>;
        const SpellStore* spellStore = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();
        sol::usertype<SpellStore> spellStoreT = lua.new_usertype<SpellStore>("ESM3_SpellStore");
        spellStoreT[sol::meta_function::to_string]
            = [](const SpellStore& store) { return "ESM3_SpellStore{" + std::to_string(store.getSize()) + " spells}"; };
        spellStoreT[sol::meta_function::length] = [](const SpellStore& store) { return store.getSize(); };
        spellStoreT[sol::meta_function::index] = sol::overload(
            [](const SpellStore& store, size_t index) -> const ESM::Spell* { return store.at(index - 1); },
            [](const SpellStore& store, std::string_view spellId) -> const ESM::Spell* {
                return store.find(ESM::RefId::deserializeText(spellId));
            });
        spellStoreT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        spellStoreT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();

        magicApi["spells"] = spellStore;

        // MagicEffect store
        using MagicEffectStore = MWWorld::Store<ESM::MagicEffect>;
        const MagicEffectStore* magicEffectStore
            = &MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>();
        auto magicEffectStoreT = lua.new_usertype<MagicEffectStore>("ESM3_MagicEffectStore");
        magicEffectStoreT[sol::meta_function::to_string] = [](const MagicEffectStore& store) {
            return "ESM3_MagicEffectStore{" + std::to_string(store.getSize()) + " effects}";
        };
        magicEffectStoreT[sol::meta_function::index]
            = [](const MagicEffectStore& store, int id) -> const ESM::MagicEffect* { return store.find(id); };
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

        // Effect params
        auto effectParamsT = lua.new_usertype<ESM::ENAMstruct>("ESM3_EffectParams");
        effectParamsT[sol::meta_function::to_string] = [magicEffectStore](const ESM::ENAMstruct& params) {
            const ESM::MagicEffect* const rec = magicEffectStore->find(params.mEffectID);
            return "ESM3_EffectParams[" + ESM::MagicEffect::effectIdToString(rec->mIndex) + "]";
        };
        effectParamsT["effect"]
            = sol::readonly_property([magicEffectStore](const ESM::ENAMstruct& params) -> const ESM::MagicEffect* {
                  return magicEffectStore->find(params.mEffectID);
              });
        effectParamsT["affectedSkill"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> sol::optional<std::string> {
                  if (params.mSkill >= 0 && params.mSkill < ESM::Skill::Length)
                      return Misc::StringUtils::lowerCase(ESM::Skill::sSkillNames[params.mSkill]);
                  else
                      return sol::nullopt;
              });
        effectParamsT["affectedAttribute"]
            = sol::readonly_property([](const ESM::ENAMstruct& params) -> sol::optional<std::string> {
                  if (params.mAttribute >= 0 && params.mAttribute < ESM::Attribute::Length)
                      return Misc::StringUtils::lowerCase(ESM::Attribute::sAttributeNames[params.mAttribute]);
                  else
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

        // MagicEffect record
        auto magicEffectT = context.mLua->sol().new_usertype<ESM::MagicEffect>("ESM3_MagicEffect");

        magicEffectT[sol::meta_function::to_string] = [](const ESM::MagicEffect& rec) {
            return "ESM3_MagicEffect[" + ESM::MagicEffect::effectIdToString(rec.mIndex) + "]";
        };
        magicEffectT["id"] = sol::readonly_property([](const ESM::MagicEffect& rec) -> int { return rec.mIndex; });
        magicEffectT["name"] = sol::readonly_property([](const ESM::MagicEffect& rec) -> std::string_view {
            return MWBase::Environment::get()
                .getWorld()
                ->getStore()
                .get<ESM::GameSetting>()
                .find(ESM::MagicEffect::effectIdToString(rec.mIndex))
                ->mValue.getString();
        });
        magicEffectT["school"]
            = sol::readonly_property([](const ESM::MagicEffect& rec) -> int { return rec.mData.mSchool; });
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

        return LuaUtil::makeReadOnly(magicApi);
    }

    // class returned via 'types.Actor.spells(obj)' in Lua
    struct ActorSpells
    {
        const ObjectVariant mActor;
    };

    void addActorMagicBindings(sol::table& actor, const Context& context)
    {
        const MWWorld::Store<ESM::Spell>* spellStore
            = &MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();

        // types.Actor.spells(o)
        actor["spells"] = [](const sol::object actor) { return ActorSpells{ ObjectVariant(actor) }; };
        auto spellsT = context.mLua->sol().new_usertype<ActorSpells>("ActorSpells");
        spellsT[sol::meta_function::to_string]
            = [](const ActorSpells& spells) { return "ActorSpells[" + spells.mActor.object().toString(); };

        // #(types.Actor.spells(o))
        spellsT[sol::meta_function::length] = [](const ActorSpells& spells) -> size_t {
            const MWWorld::Ptr& ptr = spells.mActor.ptr();
            return ptr.getClass().getCreatureStats(ptr).getSpells().count();
        };

        // types.Actor.spells(o)[i]
        spellsT[sol::meta_function::index] = sol::overload(
            [](const ActorSpells& spells, size_t index) -> const ESM::Spell* {
                const MWWorld::Ptr& ptr = spells.mActor.ptr();
                return ptr.getClass().getCreatureStats(ptr).getSpells().at(index - 1);
            },
            [spellStore](const ActorSpells& spells, std::string_view spellId) -> sol::optional<const ESM::Spell*> {
                const MWWorld::Ptr& ptr = spells.mActor.ptr();
                const ESM::Spell* spell = spellStore->find(ESM::RefId::deserializeText(spellId));
                if (ptr.getClass().getCreatureStats(ptr).getSpells().hasSpell(spell))
                    return spell;
                else
                    return sol::nullopt;
            });

        // pairs(types.Actor.spells(o))
        spellsT[sol::meta_function::pairs] = context.mLua->sol()["ipairsForArray"].template get<sol::function>();

        // ipairs(types.Actor.spells(o))
        spellsT[sol::meta_function::ipairs] = context.mLua->sol()["ipairsForArray"].template get<sol::function>();

        auto toSpellId = [](const sol::object& spellOrId) -> ESM::RefId {
            if (spellOrId.is<ESM::Spell>())
                return spellOrId.as<const ESM::Spell*>()->mId;
            else
                return ESM::RefId::deserializeText(spellOrId.as<std::string_view>());
        };

        // types.Actor.spells(o):add(id)
        spellsT["add"] = [context, toSpellId](const ActorSpells& spells, const sol::object& spellOrId) {
            if (spells.mActor.isLObject())
                throw std::runtime_error("Local scripts can modify only spells of the actor they are attached to.");
            context.mLuaManager->addAction([obj = spells.mActor.object(), id = toSpellId(spellOrId)]() {
                const MWWorld::Ptr& ptr = obj.ptr();
                ptr.getClass().getCreatureStats(ptr).getSpells().add(id);
            });
        };

        // types.Actor.spells(o):remove(id)
        spellsT["remove"] = [context, toSpellId](const ActorSpells& spells, const sol::object& spellOrId) {
            if (spells.mActor.isLObject())
                throw std::runtime_error("Local scripts can modify only spells of the actor they are attached to.");
            context.mLuaManager->addAction([obj = spells.mActor.object(), id = toSpellId(spellOrId)]() {
                const MWWorld::Ptr& ptr = obj.ptr();
                ptr.getClass().getCreatureStats(ptr).getSpells().remove(id);
            });
        };

        // types.Actor.spells(o):clear()
        spellsT["clear"] = [context](const ActorSpells& spells) {
            if (spells.mActor.isLObject())
                throw std::runtime_error("Local scripts can modify only spells of the actor they are attached to.");
            context.mLuaManager->addAction([obj = spells.mActor.object()]() {
                const MWWorld::Ptr& ptr = obj.ptr();
                ptr.getClass().getCreatureStats(ptr).getSpells().clear();
            });
        };
    }
}
