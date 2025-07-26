#include "activespells.hpp"

#include <optional>

#include <components/debug/debuglog.hpp>

#include <components/misc/resourcehelpers.hpp>

#include <components/misc/strings/algorithm.hpp>

#include <components/esm/generatedrefid.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadstat.hpp>

#include <components/settings/values.hpp>

#include "actorutil.hpp"
#include "creaturestats.hpp"
#include "spellcasting.hpp"
#include "spelleffects.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"

namespace
{
    bool merge(std::vector<ESM::ActiveEffect>& present, const std::vector<ESM::ActiveEffect>& queued)
    {
        // Can't merge if we already have an effect with the same effect index
        auto problem = std::find_if(queued.begin(), queued.end(), [&](const auto& qEffect) {
            return std::find_if(present.begin(), present.end(), [&](const auto& pEffect) {
                return pEffect.mEffectIndex == qEffect.mEffectIndex;
            }) != present.end();
        });
        if (problem != queued.end())
            return false;
        present.insert(present.end(), queued.begin(), queued.end());
        return true;
    }

    void addEffects(
        std::vector<ESM::ActiveEffect>& effects, const ESM::EffectList& list, bool ignoreResistances = false)
    {
        for (const auto& enam : list.mList)
        {
            if (enam.mData.mRange != ESM::RT_Self)
                continue;
            ESM::ActiveEffect effect;
            effect.mEffectId = enam.mData.mEffectID;
            effect.mArg = MWMechanics::EffectKey(enam.mData).mArg;
            effect.mMagnitude = 0.f;
            effect.mMinMagnitude = enam.mData.mMagnMin;
            effect.mMaxMagnitude = enam.mData.mMagnMax;
            effect.mEffectIndex = enam.mIndex;
            effect.mFlags = ESM::ActiveEffect::Flag_None;
            if (ignoreResistances)
                effect.mFlags |= ESM::ActiveEffect::Flag_Ignore_Resistances;
            effect.mDuration = -1;
            effect.mTimeLeft = -1;
            effects.emplace_back(effect);
        }
    }
}

namespace MWMechanics
{
    ActiveSpells::IterationGuard::IterationGuard(ActiveSpells& spells)
        : mActiveSpells(spells)
    {
        mActiveSpells.mIterating = true;
    }

    ActiveSpells::IterationGuard::~IterationGuard()
    {
        mActiveSpells.mIterating = false;
    }

    ActiveSpells::ActiveSpellParams::ActiveSpellParams(
        const MWWorld::Ptr& caster, const ESM::RefId& id, std::string_view sourceName, ESM::RefNum item)
        : mSourceSpellId(id)
        , mDisplayName(sourceName)
        , mCasterActorId(-1)
        , mItem(item)
        , mFlags()
        , mWorsenings(-1)
    {
        if (!caster.isEmpty() && caster.getClass().isActor())
            mCasterActorId = caster.getClass().getCreatureStats(caster).getActorId();
    }

    ActiveSpells::ActiveSpellParams::ActiveSpellParams(
        const ESM::Spell* spell, const MWWorld::Ptr& actor, bool ignoreResistances)
        : mSourceSpellId(spell->mId)
        , mDisplayName(spell->mName)
        , mCasterActorId(actor.getClass().getCreatureStats(actor).getActorId())
        , mFlags()
        , mWorsenings(-1)
    {
        assert(spell->mData.mType != ESM::Spell::ST_Spell && spell->mData.mType != ESM::Spell::ST_Power);
        setFlag(ESM::ActiveSpells::Flag_SpellStore);
        if (spell->mData.mType == ESM::Spell::ST_Ability)
            setFlag(ESM::ActiveSpells::Flag_AffectsBaseValues);
        addEffects(mEffects, spell->mEffects, ignoreResistances);
    }

    ActiveSpells::ActiveSpellParams::ActiveSpellParams(
        const MWWorld::ConstPtr& item, const ESM::Enchantment* enchantment, const MWWorld::Ptr& actor)
        : mSourceSpellId(item.getCellRef().getRefId())
        , mDisplayName(item.getClass().getName(item))
        , mCasterActorId(actor.getClass().getCreatureStats(actor).getActorId())
        , mItem(item.getCellRef().getRefNum())
        , mFlags()
        , mWorsenings(-1)
    {
        assert(enchantment->mData.mType == ESM::Enchantment::ConstantEffect);
        addEffects(mEffects, enchantment->mEffects);
        setFlag(ESM::ActiveSpells::Flag_Equipment);
    }

    ActiveSpells::ActiveSpellParams::ActiveSpellParams(const ESM::ActiveSpells::ActiveSpellParams& params)
        : mActiveSpellId(params.mActiveSpellId)
        , mSourceSpellId(params.mSourceSpellId)
        , mEffects(params.mEffects)
        , mDisplayName(params.mDisplayName)
        , mCasterActorId(params.mCasterActorId)
        , mItem(params.mItem)
        , mFlags(params.mFlags)
        , mWorsenings(params.mWorsenings)
        , mNextWorsening({ params.mNextWorsening })
    {
    }

    ActiveSpells::ActiveSpellParams::ActiveSpellParams(const ActiveSpellParams& params, const MWWorld::Ptr& actor)
        : mSourceSpellId(params.mSourceSpellId)
        , mDisplayName(params.mDisplayName)
        , mCasterActorId(actor.getClass().getCreatureStats(actor).getActorId())
        , mItem(params.mItem)
        , mFlags(params.mFlags)
        , mWorsenings(-1)
    {
    }

    ESM::ActiveSpells::ActiveSpellParams ActiveSpells::ActiveSpellParams::toEsm() const
    {
        ESM::ActiveSpells::ActiveSpellParams params;
        params.mActiveSpellId = mActiveSpellId;
        params.mSourceSpellId = mSourceSpellId;
        params.mEffects = mEffects;
        params.mDisplayName = mDisplayName;
        params.mCasterActorId = mCasterActorId;
        params.mItem = mItem;
        params.mFlags = mFlags;
        params.mWorsenings = mWorsenings;
        params.mNextWorsening = mNextWorsening.toEsm();
        return params;
    }

    void ActiveSpells::ActiveSpellParams::setFlag(ESM::ActiveSpells::Flags flag)
    {
        mFlags = static_cast<ESM::ActiveSpells::Flags>(mFlags | flag);
    }

    void ActiveSpells::ActiveSpellParams::worsen()
    {
        ++mWorsenings;
        if (!mWorsenings)
            mNextWorsening = MWBase::Environment::get().getWorld()->getTimeStamp();
        mNextWorsening += CorprusStats::sWorseningPeriod;
    }

    bool ActiveSpells::ActiveSpellParams::shouldWorsen() const
    {
        return mWorsenings >= 0 && MWBase::Environment::get().getWorld()->getTimeStamp() >= mNextWorsening;
    }

    void ActiveSpells::ActiveSpellParams::resetWorsenings()
    {
        mWorsenings = -1;
    }

    ESM::RefId ActiveSpells::ActiveSpellParams::getEnchantment() const
    {
        // Enchantment id is not stored directly. Instead the enchanted item is stored.
        const auto& store = MWBase::Environment::get().getESMStore();
        switch (store->find(mSourceSpellId))
        {
            case ESM::REC_ARMO:
                return store->get<ESM::Armor>().find(mSourceSpellId)->mEnchant;
            case ESM::REC_BOOK:
                return store->get<ESM::Book>().find(mSourceSpellId)->mEnchant;
            case ESM::REC_CLOT:
                return store->get<ESM::Clothing>().find(mSourceSpellId)->mEnchant;
            case ESM::REC_WEAP:
                return store->get<ESM::Weapon>().find(mSourceSpellId)->mEnchant;
            default:
                return {};
        }
    }

    const ESM::Spell* ActiveSpells::ActiveSpellParams::getSpell() const
    {
        return MWBase::Environment::get().getESMStore()->get<ESM::Spell>().search(getSourceSpellId());
    }

    bool ActiveSpells::ActiveSpellParams::hasFlag(ESM::ActiveSpells::Flags flags) const
    {
        return static_cast<ESM::ActiveSpells::Flags>(mFlags & flags) == flags;
    }

    void ActiveSpells::update(const MWWorld::Ptr& ptr, float duration)
    {
        if (mIterating)
            return;
        auto& creatureStats = ptr.getClass().getCreatureStats(ptr);
        assert(&creatureStats.getActiveSpells() == this);
        IterationGuard guard{ *this };
        // Erase no longer active spells and effects
        for (auto spellIt = mSpells.begin(); spellIt != mSpells.end();)
        {
            if (!spellIt->hasFlag(ESM::ActiveSpells::Flag_Temporary))
            {
                ++spellIt;
                continue;
            }
            bool removedSpell = false;
            for (auto effectIt = spellIt->mEffects.begin(); effectIt != spellIt->mEffects.end();)
            {
                if (effectIt->mFlags & ESM::ActiveEffect::Flag_Remove && effectIt->mTimeLeft <= 0.f)
                {
                    auto effect = *effectIt;
                    effectIt = spellIt->mEffects.erase(effectIt);
                    onMagicEffectRemoved(ptr, *spellIt, effect);
                    removedSpell = applyPurges(ptr, &spellIt, &effectIt);
                    if (removedSpell)
                        break;
                }
                else
                {
                    ++effectIt;
                }
            }
            if (removedSpell)
                continue;
            if (spellIt->mEffects.empty())
                spellIt = mSpells.erase(spellIt);
            else
                ++spellIt;
        }

        for (const auto& spell : mQueue)
            addToSpells(ptr, spell);
        mQueue.clear();

        // Vanilla only does this on cell change I think
        const auto& spells = creatureStats.getSpells();
        for (const ESM::Spell* spell : spells)
        {
            if (spell->mData.mType != ESM::Spell::ST_Spell && spell->mData.mType != ESM::Spell::ST_Power
                && !isSpellActive(spell->mId))
            {
                mSpells.emplace_back(ActiveSpellParams{ spell, ptr, true });
                mSpells.back().setActiveSpellId(MWBase::Environment::get().getESMStore()->generateId());
            }
        }

        bool updateSpellWindow = false;
        bool playNonLooping = false;
        if (ptr.getClass().hasInventoryStore(ptr)
            && !(creatureStats.isDead() && !creatureStats.isDeathAnimationFinished()))
        {
            auto& store = ptr.getClass().getInventoryStore(ptr);
            if (store.getInvListener() != nullptr)
            {
                playNonLooping = !store.isFirstEquip();
                const auto world = MWBase::Environment::get().getWorld();
                for (int slotIndex = 0; slotIndex < MWWorld::InventoryStore::Slots; slotIndex++)
                {
                    auto slot = store.getSlot(slotIndex);
                    if (slot == store.end())
                        continue;
                    const ESM::RefId& enchantmentId = slot->getClass().getEnchantment(*slot);
                    if (enchantmentId.empty())
                        continue;
                    const ESM::Enchantment* enchantment
                        = world->getStore().get<ESM::Enchantment>().search(enchantmentId);
                    if (enchantment == nullptr || enchantment->mData.mType != ESM::Enchantment::ConstantEffect)
                        continue;
                    if (std::find_if(mSpells.begin(), mSpells.end(),
                            [&](const ActiveSpellParams& params) {
                                return params.mItem == slot->getCellRef().getRefNum()
                                    && params.hasFlag(ESM::ActiveSpells::Flag_Equipment)
                                    && params.mSourceSpellId == slot->getCellRef().getRefId();
                            })
                        != mSpells.end())
                        continue;
                    // world->breakInvisibility leads to a stack overflow as it calls this method so just break
                    // invisibility manually
                    purgeEffect(ptr, ESM::MagicEffect::Invisibility);
                    applyPurges(ptr);
                    ActiveSpellParams& params = mSpells.emplace_back(ActiveSpellParams{ *slot, enchantment, ptr });
                    params.setActiveSpellId(MWBase::Environment::get().getESMStore()->generateId());
                    updateSpellWindow = true;
                }
            }
        }

        const MWWorld::Ptr player = MWMechanics::getPlayer();
        bool updatedHitOverlay = false;
        bool updatedEnemy = false;
        // Update effects
        for (auto spellIt = mSpells.begin(); spellIt != mSpells.end();)
        {
            const auto caster = MWBase::Environment::get().getWorld()->searchPtrViaActorId(
                spellIt->mCasterActorId); // Maybe make this search outside active grid?
            bool removedSpell = false;
            std::optional<ActiveSpellParams> reflected;
            for (auto it = spellIt->mEffects.begin(); it != spellIt->mEffects.end();)
            {
                auto result = applyMagicEffect(ptr, caster, *spellIt, *it, duration, playNonLooping);
                if (result.mType == MagicApplicationResult::Type::REFLECTED)
                {
                    if (!reflected)
                    {
                        if (Settings::game().mClassicReflectedAbsorbSpellsBehavior)
                            reflected = { *spellIt, caster };
                        else
                            reflected = { *spellIt, ptr };
                    }
                    auto& reflectedEffect = reflected->mEffects.emplace_back(*it);
                    reflectedEffect.mFlags
                        = ESM::ActiveEffect::Flag_Ignore_Reflect | ESM::ActiveEffect::Flag_Ignore_SpellAbsorption;
                    it = spellIt->mEffects.erase(it);
                }
                else if (result.mType == MagicApplicationResult::Type::REMOVED)
                    it = spellIt->mEffects.erase(it);
                else
                {
                    ++it;
                    if (!updatedEnemy && result.mShowHealth && caster == player && ptr != player)
                    {
                        MWBase::Environment::get().getWindowManager()->setEnemy(ptr);
                        updatedEnemy = true;
                    }
                    if (!updatedHitOverlay && result.mShowHit && ptr == player)
                    {
                        MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);
                        updatedHitOverlay = true;
                    }
                }
                removedSpell = applyPurges(ptr, &spellIt, &it);
                if (removedSpell)
                    break;
            }
            if (reflected)
            {
                const ESM::Static* reflectStatic = MWBase::Environment::get().getESMStore()->get<ESM::Static>().find(
                    ESM::RefId::stringRefId("VFX_Reflect"));
                MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(ptr);
                if (animation && !reflectStatic->mModel.empty())
                {
                    const VFS::Path::Normalized reflectStaticModel
                        = Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(reflectStatic->mModel));
                    animation->addEffect(
                        reflectStaticModel, ESM::MagicEffect::indexToName(ESM::MagicEffect::Reflect), false);
                }
                caster.getClass().getCreatureStats(caster).getActiveSpells().addSpell(*reflected);
            }
            if (removedSpell)
                continue;

            bool remove = false;
            if (spellIt->hasFlag(ESM::ActiveSpells::Flag_SpellStore))
            {
                try
                {
                    remove = !spells.hasSpell(spellIt->mSourceSpellId);
                }
                catch (const std::runtime_error& e)
                {
                    remove = true;
                    Log(Debug::Error) << "Removing active effect: " << e.what();
                }
            }
            else if (spellIt->hasFlag(ESM::ActiveSpells::Flag_Equipment))
            {
                // Remove effects tied to equipment that has been unequipped
                const auto& store = ptr.getClass().getInventoryStore(ptr);
                remove = true;
                for (int slotIndex = 0; slotIndex < MWWorld::InventoryStore::Slots; slotIndex++)
                {
                    auto slot = store.getSlot(slotIndex);
                    if (slot != store.end() && slot->getCellRef().getRefNum().isSet()
                        && slot->getCellRef().getRefNum() == spellIt->mItem)
                    {
                        remove = false;
                        break;
                    }
                }
            }
            if (remove)
            {
                auto params = *spellIt;
                spellIt = mSpells.erase(spellIt);
                for (const auto& effect : params.mEffects)
                    onMagicEffectRemoved(ptr, params, effect);
                applyPurges(ptr, &spellIt);
                updateSpellWindow = true;
                continue;
            }
            ++spellIt;
        }

        if (Settings::game().mClassicCalmSpellsBehavior)
        {
            ESM::MagicEffect::Effects effect
                = ptr.getClass().isNpc() ? ESM::MagicEffect::CalmHumanoid : ESM::MagicEffect::CalmCreature;
            if (creatureStats.getMagicEffects().getOrDefault(effect).getMagnitude() > 0.f)
                creatureStats.getAiSequence().stopCombat();
        }

        if (ptr == player && updateSpellWindow)
        {
            // Something happened with the spell list -- possibly while the game is paused,
            // so we want to make the spell window get the memo.
            // We don't normally want to do this, so this targets constant enchantments.
            MWBase::Environment::get().getWindowManager()->updateSpellWindow();
        }
    }

    void ActiveSpells::addToSpells(const MWWorld::Ptr& ptr, const ActiveSpellParams& spell)
    {
        if (!spell.hasFlag(ESM::ActiveSpells::Flag_Stackable))
        {
            auto found = std::find_if(mSpells.begin(), mSpells.end(), [&](const auto& existing) {
                return spell.mSourceSpellId == existing.mSourceSpellId
                    && spell.mCasterActorId == existing.mCasterActorId && spell.mItem == existing.mItem;
            });
            if (found != mSpells.end())
            {
                if (merge(found->mEffects, spell.mEffects))
                    return;
                auto params = *found;
                mSpells.erase(found);
                for (const auto& effect : params.mEffects)
                    onMagicEffectRemoved(ptr, params, effect);
            }
        }
        mSpells.emplace_back(spell);
        mSpells.back().setActiveSpellId(MWBase::Environment::get().getESMStore()->generateId());
    }

    ActiveSpells::ActiveSpells()
        : mIterating(false)
    {
    }

    ActiveSpells::TIterator ActiveSpells::begin() const
    {
        return mSpells.begin();
    }

    ActiveSpells::TIterator ActiveSpells::end() const
    {
        return mSpells.end();
    }

    ActiveSpells::TIterator ActiveSpells::getActiveSpellById(const ESM::RefId& id)
    {
        for (TIterator it = begin(); it != end(); it++)
            if (it->getActiveSpellId() == id)
                return it;
        return end();
    }

    bool ActiveSpells::isSpellActive(const ESM::RefId& id) const
    {
        return std::find_if(mSpells.begin(), mSpells.end(), [&](const auto& spell) {
            return spell.mSourceSpellId == id;
        }) != mSpells.end();
    }

    bool ActiveSpells::isEnchantmentActive(const ESM::RefId& id) const
    {
        const auto& store = MWBase::Environment::get().getESMStore();
        if (store->get<ESM::Enchantment>().search(id) == nullptr)
            return false;

        return std::find_if(mSpells.begin(), mSpells.end(), [&](const auto& spell) {
            return spell.getEnchantment() == id;
        }) != mSpells.end();
    }

    void ActiveSpells::addSpell(const ActiveSpellParams& params)
    {
        mQueue.emplace_back(params);
    }

    void ActiveSpells::addSpell(const ESM::Spell* spell, const MWWorld::Ptr& actor, bool ignoreResistances)
    {
        mQueue.emplace_back(ActiveSpellParams{ spell, actor, ignoreResistances });
    }

    void ActiveSpells::purge(ParamsPredicate predicate, const MWWorld::Ptr& ptr)
    {
        assert(&ptr.getClass().getCreatureStats(ptr).getActiveSpells() == this);
        mPurges.emplace(predicate);
        if (!mIterating)
        {
            IterationGuard guard{ *this };
            applyPurges(ptr);
        }
    }

    void ActiveSpells::purge(EffectPredicate predicate, const MWWorld::Ptr& ptr)
    {
        assert(&ptr.getClass().getCreatureStats(ptr).getActiveSpells() == this);
        mPurges.emplace(predicate);
        if (!mIterating)
        {
            IterationGuard guard{ *this };
            applyPurges(ptr);
        }
    }

    bool ActiveSpells::applyPurges(const MWWorld::Ptr& ptr, std::list<ActiveSpellParams>::iterator* currentSpell,
        std::vector<ActiveEffect>::iterator* currentEffect)
    {
        bool removedCurrentSpell = false;
        while (!mPurges.empty())
        {
            auto predicate = mPurges.front();
            mPurges.pop();
            for (auto spellIt = mSpells.begin(); spellIt != mSpells.end();)
            {
                bool isCurrentSpell = currentSpell && *currentSpell == spellIt;
                std::visit(
                    [&](auto&& variant) {
                        using T = std::decay_t<decltype(variant)>;
                        if constexpr (std::is_same_v<T, ParamsPredicate>)
                        {
                            if (variant(*spellIt))
                            {
                                auto params = *spellIt;
                                spellIt = mSpells.erase(spellIt);
                                if (isCurrentSpell)
                                {
                                    *currentSpell = spellIt;
                                    removedCurrentSpell = true;
                                }
                                for (const auto& effect : params.mEffects)
                                    onMagicEffectRemoved(ptr, params, effect);
                            }
                            else
                                ++spellIt;
                        }
                        else
                        {
                            static_assert(std::is_same_v<T, EffectPredicate>, "Non-exhaustive visitor");
                            for (auto effectIt = spellIt->mEffects.begin(); effectIt != spellIt->mEffects.end();)
                            {
                                if (variant(*spellIt, *effectIt))
                                {
                                    auto effect = *effectIt;
                                    if (isCurrentSpell && currentEffect)
                                    {
                                        auto distance = std::distance(spellIt->mEffects.begin(), *currentEffect);
                                        if (effectIt <= *currentEffect)
                                            distance--;
                                        effectIt = spellIt->mEffects.erase(effectIt);
                                        *currentEffect = spellIt->mEffects.begin() + distance;
                                    }
                                    else
                                        effectIt = spellIt->mEffects.erase(effectIt);
                                    onMagicEffectRemoved(ptr, *spellIt, effect);
                                }
                                else
                                    ++effectIt;
                            }
                            ++spellIt;
                        }
                    },
                    predicate);
            }
        }
        return removedCurrentSpell;
    }

    void ActiveSpells::removeEffectsBySourceSpellId(const MWWorld::Ptr& ptr, const ESM::RefId& id)
    {
        purge([=](const ActiveSpellParams& params) { return params.mSourceSpellId == id; }, ptr);
    }

    void ActiveSpells::removeEffectsByActiveSpellId(const MWWorld::Ptr& ptr, const ESM::RefId& id)
    {
        purge([=](const ActiveSpellParams& params) { return params.mActiveSpellId == id; }, ptr);
    }

    void ActiveSpells::purgeEffect(const MWWorld::Ptr& ptr, int effectId, ESM::RefId effectArg)
    {
        purge(
            [=](const ActiveSpellParams&, const ESM::ActiveEffect& effect) {
                if (effectArg.empty())
                    return effect.mEffectId == effectId;
                return effect.mEffectId == effectId && effect.getSkillOrAttribute() == effectArg;
            },
            ptr);
    }

    void ActiveSpells::purge(const MWWorld::Ptr& ptr, int casterActorId)
    {
        purge([=](const ActiveSpellParams& params) { return params.mCasterActorId == casterActorId; }, ptr);
    }

    void ActiveSpells::clear(const MWWorld::Ptr& ptr)
    {
        mQueue.clear();
        purge([](const ActiveSpellParams& params) { return true; }, ptr);
    }

    void ActiveSpells::skipWorsenings(double hours)
    {
        for (auto& spell : mSpells)
        {
            if (spell.mWorsenings >= 0)
                spell.mNextWorsening += hours;
        }
    }

    void ActiveSpells::writeState(ESM::ActiveSpells& state) const
    {
        for (const auto& spell : mSpells)
            state.mSpells.emplace_back(spell.toEsm());
        for (const auto& spell : mQueue)
            state.mQueue.emplace_back(spell.toEsm());
    }

    void ActiveSpells::readState(const ESM::ActiveSpells& state)
    {
        for (const ESM::ActiveSpells::ActiveSpellParams& spell : state.mSpells)
        {
            mSpells.emplace_back(ActiveSpellParams{ spell });
            // Generate ID for older saves that didn't have any.
            if (mSpells.back().getActiveSpellId().empty())
                mSpells.back().setActiveSpellId(MWBase::Environment::get().getESMStore()->generateId());
        }
        for (const ESM::ActiveSpells::ActiveSpellParams& spell : state.mQueue)
            mQueue.emplace_back(ActiveSpellParams{ spell });
    }

    void ActiveSpells::unloadActor(const MWWorld::Ptr& ptr)
    {
        purge([](const auto& spell) { return spell.hasFlag(ESM::ActiveSpells::Flag_Temporary); }, ptr);
        mQueue.clear();
    }
}
