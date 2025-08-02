#include "statsextensions.hpp"

#include <cmath>

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadnpc.hpp>

#include "../mwworld/esmstore.hpp"

#include <components/compiler/opcodes.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>

#include <components/esm3/loadmgef.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "ref.hpp"

namespace
{
    ESM::RefId getDialogueActorFaction(const MWWorld::ConstPtr& actor)
    {
        ESM::RefId factionId = actor.getClass().getPrimaryFaction(actor);
        if (factionId.empty())
            throw std::runtime_error("failed to determine dialogue actors faction (because actor is factionless)");

        return factionId;
    }

    void modStat(MWMechanics::AttributeValue& stat, float amount)
    {
        const float base = stat.getBase();
        const float modifier = stat.getModifier() - stat.getDamage();
        const float modified = base + modifier;
        // Clamp to 100 unless base < 100 and we have a fortification going
        if ((modifier <= 0.f || base >= 100.f) && amount > 0.f)
            amount = std::clamp(100.f - modified, 0.f, amount);
        // Clamp the modified value in a way that doesn't properly account for negative numbers
        float newModified = modified + amount;
        if (newModified < 0.f)
        {
            if (modified >= 0.f)
                newModified = 0.f;
            else if (newModified < modified)
                newModified = modified;
        }
        // Calculate damage/fortification based on the clamped base value
        stat.setBase(std::clamp(base + amount, 0.f, 100.f), true);
        stat.setModifier(newModified - stat.getBase());
    }

    template <class T>
    void updateBaseRecord(MWWorld::Ptr& ptr)
    {
        const auto& store = *MWBase::Environment::get().getESMStore();
        const T* base = store.get<T>().find(ptr.getCellRef().getRefId());
        ptr.get<T>()->mBase = base;
    }
}

namespace MWScript
{
    namespace Stats
    {
        template <class R>
        class OpGetLevel : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = -1;
                if (ptr.getClass().isActor())
                    value = ptr.getClass().getCreatureStats(ptr).getLevel();

                runtime.push(value);
            }
        };

        template <class R>
        class OpSetLevel : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();

                if (ptr.getClass().isActor())
                    ptr.getClass().getCreatureStats(ptr).setLevel(value);
            }
        };

        template <class R>
        class OpGetAttribute : public Interpreter::Opcode0
        {
            ESM::RefId mIndex;

        public:
            OpGetAttribute(ESM::RefId index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float value = 0.f;
                if (ptr.getClass().isActor())
                    value = ptr.getClass().getCreatureStats(ptr).getAttribute(mIndex).getModified();

                runtime.push(value);
            }
        };

        template <class R>
        class OpSetAttribute : public Interpreter::Opcode0
        {
            ESM::RefId mIndex;

        public:
            OpSetAttribute(ESM::RefId index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float value = runtime[0].mFloat;
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWMechanics::AttributeValue attribute = ptr.getClass().getCreatureStats(ptr).getAttribute(mIndex);
                attribute.setBase(value, true);
                ptr.getClass().getCreatureStats(ptr).setAttribute(mIndex, attribute);
            }
        };

        template <class R>
        class OpModAttribute : public Interpreter::Opcode0
        {
            ESM::RefId mIndex;

        public:
            OpModAttribute(ESM::RefId index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float value = runtime[0].mFloat;
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWMechanics::AttributeValue attribute = ptr.getClass().getCreatureStats(ptr).getAttribute(mIndex);
                modStat(attribute, value);
                ptr.getClass().getCreatureStats(ptr).setAttribute(mIndex, attribute);
            }
        };

        template <class R>
        class OpGetDynamic : public Interpreter::Opcode0
        {
            int mIndex;

        public:
            OpGetDynamic(int index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                Interpreter::Type_Float value = 0.f;

                if (mIndex == 0 && ptr.getClass().hasItemHealth(ptr))
                {
                    // health is a special case
                    value = static_cast<Interpreter::Type_Float>(ptr.getClass().getItemMaxHealth(ptr));
                }
                else if (ptr.getClass().isActor())
                {
                    value = ptr.getClass().getCreatureStats(ptr).getDynamic(mIndex).getCurrent();
                    // GetMagicka shouldn't return negative values
                    if (mIndex == 1 && value < 0)
                        value = 0;
                }
                runtime.push(value);
            }
        };

        template <class R>
        class OpSetDynamic : public Interpreter::Opcode0
        {
            int mIndex;

        public:
            OpSetDynamic(int index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float value = runtime[0].mFloat;
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWMechanics::DynamicStat<float> stat(ptr.getClass().getCreatureStats(ptr).getDynamic(mIndex));

                stat.setBase(value);
                stat.setCurrent(stat.getModified(false), true, true);

                ptr.getClass().getCreatureStats(ptr).setDynamic(mIndex, stat);
            }
        };

        template <class R>
        class OpModDynamic : public Interpreter::Opcode0
        {
            int mIndex;

        public:
            OpModDynamic(int index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                int peek = R::implicit ? 0 : runtime[0].mInteger;

                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float diff = runtime[0].mFloat;
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                // workaround broken endgame scripts that kill dagoth ur
                if (!R::implicit && ptr.getCellRef().getRefId() == "dagoth_ur_1")
                {
                    runtime.push(peek);

                    if (R()(runtime, false, true).isEmpty())
                    {
                        Log(Debug::Warning) << "Warning: Compensating for broken script in Morrowind.esm by "
                                            << "ignoring remote access to dagoth_ur_1";

                        return;
                    }
                }

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

                MWMechanics::DynamicStat<float> stat = stats.getDynamic(mIndex);

                float current = stat.getCurrent();
                float base = diff + stat.getBase();
                if (mIndex != 2)
                    base = std::max(base, 0.f);
                stat.setBase(base);
                stat.setCurrent(diff + current, true, true);

                stats.setDynamic(mIndex, stat);
            }
        };

        template <class R>
        class OpModCurrentDynamic : public Interpreter::Opcode0
        {
            int mIndex;

        public:
            OpModCurrentDynamic(int index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float diff = runtime[0].mFloat;
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

                Interpreter::Type_Float current = stats.getDynamic(mIndex).getCurrent();

                MWMechanics::DynamicStat<float> stat(ptr.getClass().getCreatureStats(ptr).getDynamic(mIndex));

                bool allowDecreaseBelowZero = false;
                if (mIndex == 2) // Fatigue-specific logic
                {
                    // For fatigue, a negative current value is allowed and means the actor will be knocked down
                    allowDecreaseBelowZero = true;
                    // Knock down the actor immediately if a non-positive new value is the case
                    if (diff + current <= 0.f)
                        ptr.getClass().getCreatureStats(ptr).setKnockedDown(true);
                }
                stat.setCurrent(diff + current, allowDecreaseBelowZero);

                ptr.getClass().getCreatureStats(ptr).setDynamic(mIndex, stat);
            }
        };

        template <class R>
        class OpGetDynamicGetRatio : public Interpreter::Opcode0
        {
            int mIndex;

        public:
            OpGetDynamicGetRatio(int index)
                : mIndex(index)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (!ptr.getClass().isActor())
                {
                    runtime.push(0.f);
                    return;
                }

                const MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

                runtime.push(stats.getDynamic(mIndex).getRatio(false));
            }
        };

        template <class R>
        class OpGetSkill : public Interpreter::Opcode0
        {
            ESM::RefId mId;

        public:
            OpGetSkill(ESM::RefId id)
                : mId(id)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (!ptr.getClass().isActor())
                {
                    runtime.push(0.f);
                    return;
                }

                Interpreter::Type_Float value = ptr.getClass().getSkill(ptr, mId);

                runtime.push(value);
            }
        };

        template <class R>
        class OpSetSkill : public Interpreter::Opcode0
        {
            ESM::RefId mId;

        public:
            OpSetSkill(ESM::RefId id)
                : mId(id)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float value = runtime[0].mFloat;
                runtime.pop();

                if (!ptr.getClass().isNpc())
                    return;

                MWMechanics::NpcStats& stats = ptr.getClass().getNpcStats(ptr);

                stats.getSkill(mId).setBase(value, true);
            }
        };

        template <class R>
        class OpModSkill : public Interpreter::Opcode0
        {
            ESM::RefId mId;

        public:
            OpModSkill(ESM::RefId id)
                : mId(id)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Float value = runtime[0].mFloat;
                runtime.pop();

                if (!ptr.getClass().isNpc())
                    return;

                MWMechanics::SkillValue& skill = ptr.getClass().getNpcStats(ptr).getSkill(mId);
                modStat(skill, value);
            }
        };

        class OpGetPCCrimeLevel : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                MWWorld::Ptr player = world->getPlayerPtr();
                runtime.push(static_cast<Interpreter::Type_Float>(player.getClass().getNpcStats(player).getBounty()));
            }
        };

        class OpSetPCCrimeLevel : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                MWWorld::Ptr player = world->getPlayerPtr();

                int bounty = static_cast<int>(runtime[0].mFloat);
                runtime.pop();
                player.getClass().getNpcStats(player).setBounty(bounty);

                if (bounty == 0)
                    MWBase::Environment::get().getWorld()->getPlayer().recordCrimeId();
            }
        };

        class OpModPCCrimeLevel : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::World* world = MWBase::Environment::get().getWorld();
                MWWorld::Ptr player = world->getPlayerPtr();
                int bounty = std::max(
                    0, static_cast<int>(runtime[0].mFloat) + player.getClass().getNpcStats(player).getBounty());
                player.getClass().getNpcStats(player).setBounty(bounty);
                runtime.pop();
                if (bounty == 0)
                    MWBase::Environment::get().getWorld()->getPlayer().recordCrimeId();
            }
        };

        template <class R>
        class OpAddSpell : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId id = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                const ESM::Spell* spell = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().find(id);

                MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);
                creatureStats.getSpells().add(spell);
                ESM::Spell::SpellType type = static_cast<ESM::Spell::SpellType>(spell->mData.mType);
                if (type != ESM::Spell::ST_Spell && type != ESM::Spell::ST_Power)
                {
                    // Add spell effect to *this actor's* queue immediately
                    creatureStats.getActiveSpells().addSpell(spell, ptr);
                    // Apply looping particles immediately for constant effects
                    MWBase::Environment::get().getWorld()->applyLoopingParticles(ptr);
                }
            }
        };

        template <class R>
        class OpRemoveSpell : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId id = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);
                const ESM::Spell* spell = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().find(id);
                creatureStats.getSpells().remove(spell);
                if (spell->mData.mType == ESM::Spell::ST_Ability || spell->mData.mType == ESM::Spell::ST_Blight
                    || spell->mData.mType == ESM::Spell::ST_Curse || spell->mData.mType == ESM::Spell::ST_Disease)
                    creatureStats.getActiveSpells().removeEffectsBySourceSpellId(ptr, id);

                MWBase::WindowManager* wm = MWBase::Environment::get().getWindowManager();

                if (ptr == MWMechanics::getPlayer() && id == wm->getSelectedSpell())
                {
                    wm->unsetSelectedSpell();
                }
            }
        };

        template <class R>
        class OpRemoveSpellEffects : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId spellid = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (ptr.getClass().isActor())
                    ptr.getClass().getCreatureStats(ptr).getActiveSpells().removeEffectsBySourceSpellId(ptr, spellid);
            }
        };

        template <class R>
        class OpRemoveEffects : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer effectId = runtime[0].mInteger;
                runtime.pop();

                if (ptr.getClass().isActor())
                    ptr.getClass().getCreatureStats(ptr).getActiveSpells().purgeEffect(ptr, effectId);
            }
        };

        template <class R>
        class OpGetSpell : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {

                MWWorld::Ptr ptr = R()(runtime);

                ESM::RefId id = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Integer value = 0;

                if (ptr.getClass().isActor() && ptr.getClass().getCreatureStats(ptr).getSpells().hasSpell(id))
                    value = 1;

                runtime.push(value);
            }
        };

        template <class R>
        class OpPCJoinFaction : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr actor = R()(runtime, false);

                ESM::RefId factionID;

                if (arg0 == 0)
                {
                    factionID = getDialogueActorFaction(actor);
                }
                else
                {
                    factionID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                // Make sure this faction exists
                MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionID);

                if (!factionID.empty())
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    player.getClass().getNpcStats(player).joinFaction(factionID);
                }
            }
        };

        template <class R>
        class OpPCRaiseRank : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr actor = R()(runtime, false);

                ESM::RefId factionID;

                if (arg0 == 0)
                {
                    factionID = getDialogueActorFaction(actor);
                }
                else
                {
                    factionID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                // Make sure this faction exists
                MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionID);

                if (!factionID.empty())
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    if (!player.getClass().getNpcStats(player).isInFaction(factionID))
                    {
                        player.getClass().getNpcStats(player).joinFaction(factionID);
                    }
                    else
                    {
                        int currentRank = player.getClass().getNpcStats(player).getFactionRank(factionID);
                        player.getClass().getNpcStats(player).setFactionRank(factionID, currentRank + 1);
                    }
                }
            }
        };

        template <class R>
        class OpPCLowerRank : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr actor = R()(runtime, false);

                ESM::RefId factionID;

                if (arg0 == 0)
                {
                    factionID = getDialogueActorFaction(actor);
                }
                else
                {
                    factionID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                // Make sure this faction exists
                MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionID);

                if (!factionID.empty())
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    int currentRank = player.getClass().getNpcStats(player).getFactionRank(factionID);
                    player.getClass().getNpcStats(player).setFactionRank(factionID, currentRank - 1);
                }
            }
        };

        template <class R>
        class OpGetPCRank : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr ptr = R()(runtime, false);

                ESM::RefId factionID;
                if (arg0 > 0)
                {
                    factionID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                else
                {
                    factionID = ptr.getClass().getPrimaryFaction(ptr);
                }
                // Make sure this faction exists
                MWBase::Environment::get().getESMStore()->get<ESM::Faction>().find(factionID);

                if (!factionID.empty())
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    runtime.push(player.getClass().getNpcStats(player).getFactionRank(factionID));
                }
                else
                {
                    runtime.push(-1);
                }
            }
        };

        template <class R>
        class OpModDisposition : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();

                if (ptr.getClass().isNpc())
                    ptr.getClass().getNpcStats(ptr).setBaseDisposition(
                        ptr.getClass().getNpcStats(ptr).getBaseDisposition() + value);

                // else: must not throw exception (used by an Almalexia dialogue script)
            }
        };

        template <class R>
        class OpSetDisposition : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();

                if (ptr.getClass().isNpc())
                    ptr.getClass().getNpcStats(ptr).setBaseDisposition(value);
            }
        };

        template <class R>
        class OpGetDisposition : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (!ptr.getClass().isNpc())
                    runtime.push(0);
                else
                    runtime.push(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(ptr));
            }
        };

        class OpGetDeadCount : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId id = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime[0].mInteger = MWBase::Environment::get().getMechanicsManager()->countDeaths(id);
            }
        };

        template <class R>
        class OpGetPCFacRep : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr ptr = R()(runtime, false);

                ESM::RefId factionId;

                if (arg0 == 1)
                {
                    factionId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                else
                {
                    factionId = getDialogueActorFaction(ptr);
                }

                if (factionId.empty())
                    throw std::runtime_error("failed to determine faction");

                MWWorld::Ptr player = MWMechanics::getPlayer();
                runtime.push(player.getClass().getNpcStats(player).getFactionReputation(factionId));
            }
        };

        template <class R>
        class OpSetPCFacRep : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr ptr = R()(runtime, false);

                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();

                ESM::RefId factionId;

                if (arg0 == 1)
                {
                    factionId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                else
                {
                    factionId = getDialogueActorFaction(ptr);
                }

                if (factionId.empty())
                    throw std::runtime_error("failed to determine faction");

                MWWorld::Ptr player = MWMechanics::getPlayer();
                player.getClass().getNpcStats(player).setFactionReputation(factionId, value);
            }
        };

        template <class R>
        class OpModPCFacRep : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr ptr = R()(runtime, false);

                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();

                ESM::RefId factionId;

                if (arg0 == 1)
                {
                    factionId = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                else
                {
                    factionId = getDialogueActorFaction(ptr);
                }

                if (factionId.empty())
                    throw std::runtime_error("failed to determine faction");

                MWWorld::Ptr player = MWMechanics::getPlayer();
                player.getClass().getNpcStats(player).setFactionReputation(
                    factionId, player.getClass().getNpcStats(player).getFactionReputation(factionId) + value);
            }
        };

        template <class R>
        class OpGetCommonDisease : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (ptr.getClass().isActor())
                    runtime.push(ptr.getClass().getCreatureStats(ptr).hasCommonDisease());
                else
                    runtime.push(0);
            }
        };

        template <class R>
        class OpGetBlightDisease : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (ptr.getClass().isActor())
                    runtime.push(ptr.getClass().getCreatureStats(ptr).hasBlightDisease());
                else
                    runtime.push(0);
            }
        };

        template <class R>
        class OpGetRace : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::ConstPtr ptr = R()(runtime);

                ESM::RefId race = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (ptr.getClass().isNpc())
                {
                    const ESM::RefId& npcRace = ptr.get<ESM::NPC>()->mBase->mRace;

                    runtime.push(race == npcRace);
                }
                else
                {
                    runtime.push(0);
                }
            }
        };

        class OpGetWerewolfKills : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();

                runtime.push(ptr.getClass().getNpcStats(ptr).getWerewolfKills());
            }
        };

        template <class R>
        class OpPcExpelled : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr ptr = R()(runtime, false);

                ESM::RefId factionID;
                if (arg0 > 0)
                {
                    factionID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                else
                {
                    factionID = ptr.getClass().getPrimaryFaction(ptr);
                }
                MWWorld::Ptr player = MWMechanics::getPlayer();
                if (!factionID.empty())
                {
                    runtime.push(player.getClass().getNpcStats(player).getExpelled(factionID));
                }
                else
                {
                    runtime.push(0);
                }
            }
        };

        template <class R>
        class OpPcExpell : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr ptr = R()(runtime, false);

                ESM::RefId factionID;
                if (arg0 > 0)
                {
                    factionID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                else
                {
                    factionID = ptr.getClass().getPrimaryFaction(ptr);
                }
                MWWorld::Ptr player = MWMechanics::getPlayer();
                if (!factionID.empty())
                {
                    player.getClass().getNpcStats(player).expell(factionID, true);
                }
            }
        };

        template <class R>
        class OpPcClearExpelled : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWWorld::ConstPtr ptr = R()(runtime, false);

                ESM::RefId factionID;
                if (arg0 > 0)
                {
                    factionID = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                    runtime.pop();
                }
                else
                {
                    factionID = ptr.getClass().getPrimaryFaction(ptr);
                }
                MWWorld::Ptr player = MWMechanics::getPlayer();
                if (!factionID.empty())
                    player.getClass().getNpcStats(player).clearExpelled(factionID);
            }
        };

        template <class R>
        class OpRaiseRank : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                const ESM::RefId& factionID = ptr.getClass().getPrimaryFaction(ptr);
                if (factionID.empty())
                    return;

                MWWorld::Ptr player = MWMechanics::getPlayer();

                // no-op when executed on the player
                if (ptr == player)
                    return;

                // If we already changed rank for this NPC, modify current rank in the NPC stats.
                // Otherwise take rank from base NPC record, increase it and put it to NPC data.
                int currentRank = ptr.getClass().getNpcStats(ptr).getFactionRank(factionID);
                if (currentRank >= 0)
                    ptr.getClass().getNpcStats(ptr).setFactionRank(factionID, currentRank + 1);
                else
                {
                    int rank = ptr.getClass().getPrimaryFactionRank(ptr);
                    ptr.getClass().getNpcStats(ptr).joinFaction(factionID);
                    ptr.getClass().getNpcStats(ptr).setFactionRank(factionID, rank + 1);
                }
            }
        };

        template <class R>
        class OpLowerRank : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                const ESM::RefId& factionID = ptr.getClass().getPrimaryFaction(ptr);
                if (factionID.empty())
                    return;

                MWWorld::Ptr player = MWMechanics::getPlayer();

                // no-op when executed on the player
                if (ptr == player)
                    return;

                // If we already changed rank for this NPC, modify current rank in the NPC stats.
                // Otherwise take rank from base NPC record, decrease it and put it to NPC data.
                int currentRank = ptr.getClass().getNpcStats(ptr).getFactionRank(factionID);
                if (currentRank == 0)
                    return;
                else if (currentRank > 0)
                    ptr.getClass().getNpcStats(ptr).setFactionRank(factionID, currentRank - 1);
                else
                {
                    int rank = ptr.getClass().getPrimaryFactionRank(ptr);
                    ptr.getClass().getNpcStats(ptr).joinFaction(factionID);
                    ptr.getClass().getNpcStats(ptr).setFactionRank(factionID, std::max(0, rank - 1));
                }
            }
        };

        template <class R>
        class OpOnDeath : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = 0;
                if (ptr.getClass().isActor())
                {
                    auto& stats = ptr.getClass().getCreatureStats(ptr);
                    value = stats.hasDied();

                    if (value)
                        stats.clearHasDied();
                }

                runtime.push(value);
            }
        };

        template <class R>
        class OpOnMurder : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = 0;
                if (ptr.getClass().isActor())
                {
                    auto& stats = ptr.getClass().getCreatureStats(ptr);
                    value = stats.hasBeenMurdered();

                    if (value)
                        stats.clearHasBeenMurdered();
                }

                runtime.push(value);
            }
        };

        template <class R>
        class OpOnKnockout : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                Interpreter::Type_Integer value = 0;
                if (ptr.getClass().isActor())
                    value = ptr.getClass().getCreatureStats(ptr).getKnockedDownOneFrame();

                runtime.push(value);
            }
        };

        template <class R>
        class OpIsWerewolf : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                if (ptr.getClass().isNpc())
                    runtime.push(ptr.getClass().getNpcStats(ptr).isWerewolf());
                else
                    runtime.push(0);
            }
        };

        template <class R, bool set>
        class OpSetWerewolf : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                if (ptr.getClass().isNpc())
                    MWBase::Environment::get().getMechanicsManager()->setWerewolf(ptr, set);
            }
        };

        template <class R>
        class OpSetWerewolfAcrobatics : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                if (ptr.getClass().isNpc())
                    MWBase::Environment::get().getMechanicsManager()->applyWerewolfAcrobatics(ptr);
            }
        };

        template <class R>
        class OpResurrect : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (!ptr.getClass().isActor())
                    return;

                if (ptr == MWMechanics::getPlayer())
                {
                    MWBase::Environment::get().getMechanicsManager()->resurrect(ptr);
                    if (MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_Ended)
                        MWBase::Environment::get().getStateManager()->resumeGame();
                }
                else if (ptr.getClass().getCreatureStats(ptr).isDead())
                {
                    bool wasEnabled = ptr.getRefData().isEnabled();
                    MWBase::Environment::get().getWorld()->undeleteObject(ptr);
                    auto windowManager = MWBase::Environment::get().getWindowManager();
                    bool wasOpen = windowManager->containsMode(MWGui::GM_Container);
                    windowManager->onDeleteCustomData(ptr);
                    // HACK: disable/enable object to re-add it to the scene properly (need a new Animation).
                    MWBase::Environment::get().getWorld()->disable(ptr);
                    // The actor's base record may have changed after this specific reference was created.
                    // So we need to update to the current version
                    if (ptr.getClass().isNpc())
                        updateBaseRecord<ESM::NPC>(ptr);
                    else
                        updateBaseRecord<ESM::Creature>(ptr);
                    if (wasOpen && !windowManager->containsMode(MWGui::GM_Container))
                    {
                        // Reopen the loot GUI if it was closed because we resurrected the actor we were looting
                        MWBase::Environment::get().getMechanicsManager()->resurrect(ptr);
                        windowManager->forceLootMode(ptr);
                    }
                    else
                    {
                        MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
                        // resets runtime state such as inventory, stats and AI. does not reset position in the world
                        ptr.getRefData().setCustomData(nullptr);
                    }
                    if (wasEnabled)
                        MWBase::Environment::get().getWorld()->enable(ptr);
                }
            }
        };

        template <class R>
        class OpGetStat : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                // dummy
                runtime.pop();
                runtime.push(0);
            }
        };

        template <class R>
        class OpGetMagicEffect : public Interpreter::Opcode0
        {
            int mPositiveEffect;
            int mNegativeEffect;

        public:
            OpGetMagicEffect(int positiveEffect, int negativeEffect)
                : mPositiveEffect(positiveEffect)
                , mNegativeEffect(negativeEffect)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (!ptr.getClass().isActor())
                {
                    runtime.push(0);
                    return;
                }

                const MWMechanics::MagicEffects& effects = ptr.getClass().getCreatureStats(ptr).getMagicEffects();
                float currentValue = effects.getOrDefault(mPositiveEffect).getMagnitude();
                if (mNegativeEffect != -1)
                    currentValue -= effects.getOrDefault(mNegativeEffect).getMagnitude();

                // GetResist* should take in account elemental shields
                if (mPositiveEffect == ESM::MagicEffect::ResistFire)
                    currentValue += effects.getOrDefault(ESM::MagicEffect::FireShield).getMagnitude();
                if (mPositiveEffect == ESM::MagicEffect::ResistShock)
                    currentValue += effects.getOrDefault(ESM::MagicEffect::LightningShield).getMagnitude();
                if (mPositiveEffect == ESM::MagicEffect::ResistFrost)
                    currentValue += effects.getOrDefault(ESM::MagicEffect::FrostShield).getMagnitude();

                int ret = static_cast<int>(currentValue);
                runtime.push(ret);
            }
        };

        template <class R>
        class OpSetMagicEffect : public Interpreter::Opcode0
        {
            int mPositiveEffect;
            int mNegativeEffect;

        public:
            OpSetMagicEffect(int positiveEffect, int negativeEffect)
                : mPositiveEffect(positiveEffect)
                , mNegativeEffect(negativeEffect)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                int arg = runtime[0].mInteger;
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWMechanics::MagicEffects& effects = ptr.getClass().getCreatureStats(ptr).getMagicEffects();
                float currentValue = effects.getOrDefault(mPositiveEffect).getMagnitude();
                if (mNegativeEffect != -1)
                    currentValue -= effects.getOrDefault(mNegativeEffect).getMagnitude();

                // SetResist* should take in account elemental shields
                if (mPositiveEffect == ESM::MagicEffect::ResistFire)
                    currentValue += effects.getOrDefault(ESM::MagicEffect::FireShield).getMagnitude();
                if (mPositiveEffect == ESM::MagicEffect::ResistShock)
                    currentValue += effects.getOrDefault(ESM::MagicEffect::LightningShield).getMagnitude();
                if (mPositiveEffect == ESM::MagicEffect::ResistFrost)
                    currentValue += effects.getOrDefault(ESM::MagicEffect::FrostShield).getMagnitude();

                effects.modifyBase(mPositiveEffect, (arg - static_cast<int>(currentValue)));
            }
        };

        template <class R>
        class OpModMagicEffect : public Interpreter::Opcode0
        {
            int mPositiveEffect;
            int mNegativeEffect;

        public:
            OpModMagicEffect(int positiveEffect, int negativeEffect)
                : mPositiveEffect(positiveEffect)
                , mNegativeEffect(negativeEffect)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                int arg = runtime[0].mInteger;
                runtime.pop();

                if (!ptr.getClass().isActor())
                    return;

                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);
                stats.getMagicEffects().modifyBase(mPositiveEffect, arg);
            }
        };

        class OpGetPCVisionBonus : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr player = MWMechanics::getPlayer();
                MWMechanics::EffectParam nightEye
                    = player.getClass().getCreatureStats(player).getMagicEffects().getOrDefault(
                        ESM::MagicEffect::NightEye);
                runtime.push(std::clamp(nightEye.getMagnitude() / 100.f, 0.f, 1.f));
            }
        };

        class OpSetPCVisionBonus : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                float arg = runtime[0].mFloat;
                runtime.pop();
                MWWorld::Ptr player = MWMechanics::getPlayer();
                auto& effects = player.getClass().getCreatureStats(player).getMagicEffects();
                float delta = std::clamp(arg * 100.f, 0.f, 100.f)
                    - effects.getOrDefault(ESM::MagicEffect::NightEye).getMagnitude();
                effects.modifyBase(ESM::MagicEffect::NightEye, static_cast<int>(delta));
            }
        };

        class OpModPCVisionBonus : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                float arg = runtime[0].mFloat;
                runtime.pop();
                MWWorld::Ptr player = MWMechanics::getPlayer();
                auto& effects = player.getClass().getCreatureStats(player).getMagicEffects();
                const MWMechanics::EffectParam nightEye = effects.getOrDefault(ESM::MagicEffect::NightEye);
                float newBase = std::clamp(nightEye.getMagnitude() + arg * 100.f, 0.f, 100.f);
                newBase -= nightEye.getModifier();
                float delta = std::clamp(newBase, 0.f, 100.f) - nightEye.getMagnitude();
                effects.modifyBase(ESM::MagicEffect::NightEye, static_cast<int>(delta));
            }
        };

        struct MagicEffect
        {
            int mPositiveEffect;
            int mNegativeEffect;
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            for (int i = 0; i < Compiler::Stats::numberOfAttributes; ++i)
            {
                ESM::RefId id = ESM::Attribute::indexToRefId(i);
                interpreter.installSegment5<OpGetAttribute<ImplicitRef>>(Compiler::Stats::opcodeGetAttribute + i, id);
                interpreter.installSegment5<OpGetAttribute<ExplicitRef>>(
                    Compiler::Stats::opcodeGetAttributeExplicit + i, id);

                interpreter.installSegment5<OpSetAttribute<ImplicitRef>>(Compiler::Stats::opcodeSetAttribute + i, id);
                interpreter.installSegment5<OpSetAttribute<ExplicitRef>>(
                    Compiler::Stats::opcodeSetAttributeExplicit + i, id);

                interpreter.installSegment5<OpModAttribute<ImplicitRef>>(Compiler::Stats::opcodeModAttribute + i, id);
                interpreter.installSegment5<OpModAttribute<ExplicitRef>>(
                    Compiler::Stats::opcodeModAttributeExplicit + i, id);
            }

            for (int i = 0; i < Compiler::Stats::numberOfDynamics; ++i)
            {
                interpreter.installSegment5<OpGetDynamic<ImplicitRef>>(Compiler::Stats::opcodeGetDynamic + i, i);
                interpreter.installSegment5<OpGetDynamic<ExplicitRef>>(
                    Compiler::Stats::opcodeGetDynamicExplicit + i, i);

                interpreter.installSegment5<OpSetDynamic<ImplicitRef>>(Compiler::Stats::opcodeSetDynamic + i, i);
                interpreter.installSegment5<OpSetDynamic<ExplicitRef>>(
                    Compiler::Stats::opcodeSetDynamicExplicit + i, i);

                interpreter.installSegment5<OpModDynamic<ImplicitRef>>(Compiler::Stats::opcodeModDynamic + i, i);
                interpreter.installSegment5<OpModDynamic<ExplicitRef>>(
                    Compiler::Stats::opcodeModDynamicExplicit + i, i);

                interpreter.installSegment5<OpModCurrentDynamic<ImplicitRef>>(
                    Compiler::Stats::opcodeModCurrentDynamic + i, i);
                interpreter.installSegment5<OpModCurrentDynamic<ExplicitRef>>(
                    Compiler::Stats::opcodeModCurrentDynamicExplicit + i, i);

                interpreter.installSegment5<OpGetDynamicGetRatio<ImplicitRef>>(
                    Compiler::Stats::opcodeGetDynamicGetRatio + i, i);
                interpreter.installSegment5<OpGetDynamicGetRatio<ExplicitRef>>(
                    Compiler::Stats::opcodeGetDynamicGetRatioExplicit + i, i);
            }

            for (int i = 0; i < Compiler::Stats::numberOfSkills; ++i)
            {
                ESM::RefId id = ESM::Skill::indexToRefId(i);
                interpreter.installSegment5<OpGetSkill<ImplicitRef>>(Compiler::Stats::opcodeGetSkill + i, id);
                interpreter.installSegment5<OpGetSkill<ExplicitRef>>(Compiler::Stats::opcodeGetSkillExplicit + i, id);

                interpreter.installSegment5<OpSetSkill<ImplicitRef>>(Compiler::Stats::opcodeSetSkill + i, id);
                interpreter.installSegment5<OpSetSkill<ExplicitRef>>(Compiler::Stats::opcodeSetSkillExplicit + i, id);

                interpreter.installSegment5<OpModSkill<ImplicitRef>>(Compiler::Stats::opcodeModSkill + i, id);
                interpreter.installSegment5<OpModSkill<ExplicitRef>>(Compiler::Stats::opcodeModSkillExplicit + i, id);
            }

            interpreter.installSegment5<OpGetPCCrimeLevel>(Compiler::Stats::opcodeGetPCCrimeLevel);
            interpreter.installSegment5<OpSetPCCrimeLevel>(Compiler::Stats::opcodeSetPCCrimeLevel);
            interpreter.installSegment5<OpModPCCrimeLevel>(Compiler::Stats::opcodeModPCCrimeLevel);

            interpreter.installSegment5<OpAddSpell<ImplicitRef>>(Compiler::Stats::opcodeAddSpell);
            interpreter.installSegment5<OpAddSpell<ExplicitRef>>(Compiler::Stats::opcodeAddSpellExplicit);
            interpreter.installSegment5<OpRemoveSpell<ImplicitRef>>(Compiler::Stats::opcodeRemoveSpell);
            interpreter.installSegment5<OpRemoveSpell<ExplicitRef>>(Compiler::Stats::opcodeRemoveSpellExplicit);
            interpreter.installSegment5<OpRemoveSpellEffects<ImplicitRef>>(Compiler::Stats::opcodeRemoveSpellEffects);
            interpreter.installSegment5<OpRemoveSpellEffects<ExplicitRef>>(
                Compiler::Stats::opcodeRemoveSpellEffectsExplicit);
            interpreter.installSegment5<OpResurrect<ImplicitRef>>(Compiler::Stats::opcodeResurrect);
            interpreter.installSegment5<OpResurrect<ExplicitRef>>(Compiler::Stats::opcodeResurrectExplicit);
            interpreter.installSegment5<OpRemoveEffects<ImplicitRef>>(Compiler::Stats::opcodeRemoveEffects);
            interpreter.installSegment5<OpRemoveEffects<ExplicitRef>>(Compiler::Stats::opcodeRemoveEffectsExplicit);

            interpreter.installSegment5<OpGetSpell<ImplicitRef>>(Compiler::Stats::opcodeGetSpell);
            interpreter.installSegment5<OpGetSpell<ExplicitRef>>(Compiler::Stats::opcodeGetSpellExplicit);

            interpreter.installSegment3<OpPCRaiseRank<ImplicitRef>>(Compiler::Stats::opcodePCRaiseRank);
            interpreter.installSegment3<OpPCLowerRank<ImplicitRef>>(Compiler::Stats::opcodePCLowerRank);
            interpreter.installSegment3<OpPCJoinFaction<ImplicitRef>>(Compiler::Stats::opcodePCJoinFaction);
            interpreter.installSegment3<OpPCRaiseRank<ExplicitRef>>(Compiler::Stats::opcodePCRaiseRankExplicit);
            interpreter.installSegment3<OpPCLowerRank<ExplicitRef>>(Compiler::Stats::opcodePCLowerRankExplicit);
            interpreter.installSegment3<OpPCJoinFaction<ExplicitRef>>(Compiler::Stats::opcodePCJoinFactionExplicit);
            interpreter.installSegment3<OpGetPCRank<ImplicitRef>>(Compiler::Stats::opcodeGetPCRank);
            interpreter.installSegment3<OpGetPCRank<ExplicitRef>>(Compiler::Stats::opcodeGetPCRankExplicit);

            interpreter.installSegment5<OpModDisposition<ImplicitRef>>(Compiler::Stats::opcodeModDisposition);
            interpreter.installSegment5<OpModDisposition<ExplicitRef>>(Compiler::Stats::opcodeModDispositionExplicit);
            interpreter.installSegment5<OpSetDisposition<ImplicitRef>>(Compiler::Stats::opcodeSetDisposition);
            interpreter.installSegment5<OpSetDisposition<ExplicitRef>>(Compiler::Stats::opcodeSetDispositionExplicit);
            interpreter.installSegment5<OpGetDisposition<ImplicitRef>>(Compiler::Stats::opcodeGetDisposition);
            interpreter.installSegment5<OpGetDisposition<ExplicitRef>>(Compiler::Stats::opcodeGetDispositionExplicit);

            interpreter.installSegment5<OpGetLevel<ImplicitRef>>(Compiler::Stats::opcodeGetLevel);
            interpreter.installSegment5<OpGetLevel<ExplicitRef>>(Compiler::Stats::opcodeGetLevelExplicit);
            interpreter.installSegment5<OpSetLevel<ImplicitRef>>(Compiler::Stats::opcodeSetLevel);
            interpreter.installSegment5<OpSetLevel<ExplicitRef>>(Compiler::Stats::opcodeSetLevelExplicit);

            interpreter.installSegment5<OpGetDeadCount>(Compiler::Stats::opcodeGetDeadCount);

            interpreter.installSegment3<OpGetPCFacRep<ImplicitRef>>(Compiler::Stats::opcodeGetPCFacRep);
            interpreter.installSegment3<OpGetPCFacRep<ExplicitRef>>(Compiler::Stats::opcodeGetPCFacRepExplicit);
            interpreter.installSegment3<OpSetPCFacRep<ImplicitRef>>(Compiler::Stats::opcodeSetPCFacRep);
            interpreter.installSegment3<OpSetPCFacRep<ExplicitRef>>(Compiler::Stats::opcodeSetPCFacRepExplicit);
            interpreter.installSegment3<OpModPCFacRep<ImplicitRef>>(Compiler::Stats::opcodeModPCFacRep);
            interpreter.installSegment3<OpModPCFacRep<ExplicitRef>>(Compiler::Stats::opcodeModPCFacRepExplicit);

            interpreter.installSegment5<OpGetCommonDisease<ImplicitRef>>(Compiler::Stats::opcodeGetCommonDisease);
            interpreter.installSegment5<OpGetCommonDisease<ExplicitRef>>(
                Compiler::Stats::opcodeGetCommonDiseaseExplicit);
            interpreter.installSegment5<OpGetBlightDisease<ImplicitRef>>(Compiler::Stats::opcodeGetBlightDisease);
            interpreter.installSegment5<OpGetBlightDisease<ExplicitRef>>(
                Compiler::Stats::opcodeGetBlightDiseaseExplicit);

            interpreter.installSegment5<OpGetRace<ImplicitRef>>(Compiler::Stats::opcodeGetRace);
            interpreter.installSegment5<OpGetRace<ExplicitRef>>(Compiler::Stats::opcodeGetRaceExplicit);
            interpreter.installSegment5<OpGetWerewolfKills>(Compiler::Stats::opcodeGetWerewolfKills);

            interpreter.installSegment3<OpPcExpelled<ImplicitRef>>(Compiler::Stats::opcodePcExpelled);
            interpreter.installSegment3<OpPcExpelled<ExplicitRef>>(Compiler::Stats::opcodePcExpelledExplicit);
            interpreter.installSegment3<OpPcExpell<ImplicitRef>>(Compiler::Stats::opcodePcExpell);
            interpreter.installSegment3<OpPcExpell<ExplicitRef>>(Compiler::Stats::opcodePcExpellExplicit);
            interpreter.installSegment3<OpPcClearExpelled<ImplicitRef>>(Compiler::Stats::opcodePcClearExpelled);
            interpreter.installSegment3<OpPcClearExpelled<ExplicitRef>>(Compiler::Stats::opcodePcClearExpelledExplicit);
            interpreter.installSegment5<OpRaiseRank<ImplicitRef>>(Compiler::Stats::opcodeRaiseRank);
            interpreter.installSegment5<OpRaiseRank<ExplicitRef>>(Compiler::Stats::opcodeRaiseRankExplicit);
            interpreter.installSegment5<OpLowerRank<ImplicitRef>>(Compiler::Stats::opcodeLowerRank);
            interpreter.installSegment5<OpLowerRank<ExplicitRef>>(Compiler::Stats::opcodeLowerRankExplicit);

            interpreter.installSegment5<OpOnDeath<ImplicitRef>>(Compiler::Stats::opcodeOnDeath);
            interpreter.installSegment5<OpOnDeath<ExplicitRef>>(Compiler::Stats::opcodeOnDeathExplicit);
            interpreter.installSegment5<OpOnMurder<ImplicitRef>>(Compiler::Stats::opcodeOnMurder);
            interpreter.installSegment5<OpOnMurder<ExplicitRef>>(Compiler::Stats::opcodeOnMurderExplicit);
            interpreter.installSegment5<OpOnKnockout<ImplicitRef>>(Compiler::Stats::opcodeOnKnockout);
            interpreter.installSegment5<OpOnKnockout<ExplicitRef>>(Compiler::Stats::opcodeOnKnockoutExplicit);

            interpreter.installSegment5<OpIsWerewolf<ImplicitRef>>(Compiler::Stats::opcodeIsWerewolf);
            interpreter.installSegment5<OpIsWerewolf<ExplicitRef>>(Compiler::Stats::opcodeIsWerewolfExplicit);

            interpreter.installSegment5<OpSetWerewolf<ImplicitRef, true>>(Compiler::Stats::opcodeBecomeWerewolf);
            interpreter.installSegment5<OpSetWerewolf<ExplicitRef, true>>(
                Compiler::Stats::opcodeBecomeWerewolfExplicit);
            interpreter.installSegment5<OpSetWerewolf<ImplicitRef, false>>(Compiler::Stats::opcodeUndoWerewolf);
            interpreter.installSegment5<OpSetWerewolf<ExplicitRef, false>>(Compiler::Stats::opcodeUndoWerewolfExplicit);
            interpreter.installSegment5<OpSetWerewolfAcrobatics<ImplicitRef>>(
                Compiler::Stats::opcodeSetWerewolfAcrobatics);
            interpreter.installSegment5<OpSetWerewolfAcrobatics<ExplicitRef>>(
                Compiler::Stats::opcodeSetWerewolfAcrobaticsExplicit);
            interpreter.installSegment5<OpGetStat<ImplicitRef>>(Compiler::Stats::opcodeGetStat);
            interpreter.installSegment5<OpGetStat<ExplicitRef>>(Compiler::Stats::opcodeGetStatExplicit);

            static const MagicEffect sMagicEffects[] = {
                { ESM::MagicEffect::ResistMagicka, ESM::MagicEffect::WeaknessToMagicka },
                { ESM::MagicEffect::ResistFire, ESM::MagicEffect::WeaknessToFire },
                { ESM::MagicEffect::ResistFrost, ESM::MagicEffect::WeaknessToFrost },
                { ESM::MagicEffect::ResistShock, ESM::MagicEffect::WeaknessToShock },
                { ESM::MagicEffect::ResistCommonDisease, ESM::MagicEffect::WeaknessToCommonDisease },
                { ESM::MagicEffect::ResistBlightDisease, ESM::MagicEffect::WeaknessToBlightDisease },
                { ESM::MagicEffect::ResistCorprusDisease, ESM::MagicEffect::WeaknessToCorprusDisease },
                { ESM::MagicEffect::ResistPoison, ESM::MagicEffect::WeaknessToPoison },
                { ESM::MagicEffect::ResistParalysis, -1 },
                { ESM::MagicEffect::ResistNormalWeapons, ESM::MagicEffect::WeaknessToNormalWeapons },
                { ESM::MagicEffect::WaterBreathing, -1 },
                { ESM::MagicEffect::Chameleon, -1 },
                { ESM::MagicEffect::WaterWalking, -1 },
                { ESM::MagicEffect::SwiftSwim, -1 },
                { ESM::MagicEffect::Jump, -1 },
                { ESM::MagicEffect::Levitate, -1 },
                { ESM::MagicEffect::Shield, -1 },
                { ESM::MagicEffect::Sound, -1 },
                { ESM::MagicEffect::Silence, -1 },
                { ESM::MagicEffect::Blind, -1 },
                { ESM::MagicEffect::Paralyze, -1 },
                { ESM::MagicEffect::Invisibility, -1 },
                { ESM::MagicEffect::FortifyAttack, -1 },
                { ESM::MagicEffect::Sanctuary, -1 },
            };

            for (int i = 0; i < 24; ++i)
            {
                int positive = sMagicEffects[i].mPositiveEffect;
                int negative = sMagicEffects[i].mNegativeEffect;

                interpreter.installSegment5<OpGetMagicEffect<ImplicitRef>>(
                    Compiler::Stats::opcodeGetMagicEffect + i, positive, negative);
                interpreter.installSegment5<OpGetMagicEffect<ExplicitRef>>(
                    Compiler::Stats::opcodeGetMagicEffectExplicit + i, positive, negative);

                interpreter.installSegment5<OpSetMagicEffect<ImplicitRef>>(
                    Compiler::Stats::opcodeSetMagicEffect + i, positive, negative);
                interpreter.installSegment5<OpSetMagicEffect<ExplicitRef>>(
                    Compiler::Stats::opcodeSetMagicEffectExplicit + i, positive, negative);

                interpreter.installSegment5<OpModMagicEffect<ImplicitRef>>(
                    Compiler::Stats::opcodeModMagicEffect + i, positive, negative);
                interpreter.installSegment5<OpModMagicEffect<ExplicitRef>>(
                    Compiler::Stats::opcodeModMagicEffectExplicit + i, positive, negative);
            }

            interpreter.installSegment5<OpGetPCVisionBonus>(Compiler::Stats::opcodeGetPCVisionBonus);
            interpreter.installSegment5<OpSetPCVisionBonus>(Compiler::Stats::opcodeSetPCVisionBonus);
            interpreter.installSegment5<OpModPCVisionBonus>(Compiler::Stats::opcodeModPCVisionBonus);
        }
    }
}
