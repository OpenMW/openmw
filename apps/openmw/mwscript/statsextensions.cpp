#include "statsextensions.hpp"

#include <cmath>

#include <components/esm/loadnpc.hpp>

#include "../mwworld/esmstore.hpp"

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>
#include <components/debug/debuglog.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "ref.hpp"

namespace
{
    std::string getDialogueActorFaction(MWWorld::ConstPtr actor)
    {
        std::string factionId = actor.getClass().getPrimaryFaction(actor);
        if (factionId.empty())
            throw std::runtime_error (
                "failed to determine dialogue actors faction (because actor is factionless)");

        return factionId;
    }
}

namespace MWScript
{
    namespace Stats
    {
        template<class R>
        class OpGetLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value =
                        ptr.getClass()
                            .getCreatureStats (ptr)
                            .getLevel();

                    runtime.push (value);
                }
        };

        template<class R>
        class OpSetLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    ptr.getClass()
                        .getCreatureStats (ptr)
                        .setLevel(value);
                }
        };

        template<class R>
        class OpGetAttribute : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetAttribute (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value =
                        ptr.getClass()
                            .getCreatureStats (ptr)
                            .getAttribute(mIndex)
                            .getModified();

                    runtime.push (value);
                }
        };

        template<class R>
        class OpSetAttribute : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetAttribute (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWMechanics::AttributeValue attribute = ptr.getClass().getCreatureStats(ptr).getAttribute(mIndex);
                    attribute.setBase (value);
                    ptr.getClass().getCreatureStats(ptr).setAttribute(mIndex, attribute);
                }
        };

        template<class R>
        class OpModAttribute : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModAttribute (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWMechanics::AttributeValue attribute = ptr.getClass()
                        .getCreatureStats(ptr)
                        .getAttribute(mIndex);

                    if (value == 0)
                        return;

                    if (((attribute.getBase() <= 0) && (value < 0))
                        || ((attribute.getBase() >= 100) && (value > 0)))
                        return;

                    if (value < 0)
                        attribute.setBase(std::max(0, attribute.getBase() + value));
                    else
                        attribute.setBase(std::min(100, attribute.getBase() + value));

                    ptr.getClass().getCreatureStats(ptr).setAttribute(mIndex, attribute);
                }
        };

        template<class R>
        class OpGetDynamic : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetDynamic (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    Interpreter::Type_Float value;

                    if (mIndex==0 && ptr.getClass().hasItemHealth (ptr))
                    {
                        // health is a special case
                        value = static_cast<Interpreter::Type_Float>(ptr.getClass().getItemMaxHealth(ptr));
                    } else {
                        value =
                            ptr.getClass()
                                .getCreatureStats(ptr)
                                .getDynamic(mIndex)
                                .getCurrent();
                    }
                    runtime.push (value);
                }
        };

        template<class R>
        class OpSetDynamic : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetDynamic (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Float value = runtime[0].mFloat;
                    runtime.pop();

                    MWMechanics::DynamicStat<float> stat (ptr.getClass().getCreatureStats (ptr)
                        .getDynamic (mIndex));

                    stat.setModified (value, 0);
                    stat.setCurrent(value);

                    ptr.getClass().getCreatureStats (ptr).setDynamic (mIndex, stat);
                }
        };

        template<class R>
        class OpModDynamic : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModDynamic (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    int peek = R::implicit ? 0 : runtime[0].mInteger;

                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Float diff = runtime[0].mFloat;
                    runtime.pop();

                    // workaround broken endgame scripts that kill dagoth ur
                    if (!R::implicit &&
                        ::Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "dagoth_ur_1"))
                    {
                        runtime.push (peek);

                        if (R()(runtime, false, true).isEmpty())
                        {
                            Log(Debug::Warning)
                                << "Warning: Compensating for broken script in Morrowind.esm by "
                                << "ignoring remote access to dagoth_ur_1";

                            return;
                        }
                    }

                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);

                    Interpreter::Type_Float current = stats.getDynamic(mIndex).getCurrent();

                    MWMechanics::DynamicStat<float> stat (ptr.getClass().getCreatureStats (ptr)
                        .getDynamic (mIndex));

                    stat.setModified (diff + stat.getModified(), 0);
                    stat.setCurrentModified (diff + stat.getCurrentModified());

                    stat.setCurrent (diff + current);

                    ptr.getClass().getCreatureStats (ptr).setDynamic (mIndex, stat);
                }
        };

        template<class R>
        class OpModCurrentDynamic : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModCurrentDynamic (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Float diff = runtime[0].mFloat;
                    runtime.pop();

                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);

                    Interpreter::Type_Float current = stats.getDynamic(mIndex).getCurrent();

                    MWMechanics::DynamicStat<float> stat (ptr.getClass().getCreatureStats (ptr)
                        .getDynamic (mIndex));

                    bool allowDecreaseBelowZero = false;
                    if (mIndex == 2) // Fatigue-specific logic
                    {
                        // For fatigue, a negative current value is allowed and means the actor will be knocked down
                        allowDecreaseBelowZero = true;
                        // Knock down the actor immediately if a non-positive new value is the case
                        if (diff + current <= 0.f)
                            ptr.getClass().getCreatureStats(ptr).setKnockedDown(true);
                    }
                    stat.setCurrent (diff + current, allowDecreaseBelowZero);

                    ptr.getClass().getCreatureStats (ptr).setDynamic (mIndex, stat);
                }
        };

        template<class R>
        class OpGetDynamicGetRatio : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetDynamicGetRatio (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);

                    Interpreter::Type_Float value = 0;

                    Interpreter::Type_Float max = stats.getDynamic(mIndex).getModified();

                    if (max>0)
                        value = stats.getDynamic(mIndex).getCurrent() / max;

                    runtime.push (value);
                }
        };

        template<class R>
        class OpGetSkill : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetSkill (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = ptr.getClass().getSkill(ptr, mIndex);

                    runtime.push (value);
                }
        };

        template<class R>
        class OpSetSkill : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetSkill (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWMechanics::NpcStats& stats = ptr.getClass().getNpcStats (ptr);

                    stats.getSkill (mIndex).setBase (value);
                }
        };

        template<class R>
        class OpModSkill : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModSkill (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWMechanics::SkillValue &skill = ptr.getClass()
                        .getNpcStats(ptr)
                        .getSkill(mIndex);

                    if (value == 0)
                        return;

                    if (((skill.getBase() <= 0) && (value < 0))
                        || ((skill.getBase() >= 100) && (value > 0)))
                        return;

                    if (value < 0)
                        skill.setBase(std::max(0, skill.getBase() + value));
                    else
                        skill.setBase(std::min(100, skill.getBase() + value));
                }
        };

        class OpGetPCCrimeLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    MWWorld::Ptr player = world->getPlayerPtr();
                    runtime.push (static_cast <Interpreter::Type_Float> (player.getClass().getNpcStats (player).getBounty()));
                }
        };

        class OpSetPCCrimeLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    MWWorld::Ptr player = world->getPlayerPtr();

                    int bounty = static_cast<int>(runtime[0].mFloat);
                    runtime.pop();
                    player.getClass().getNpcStats (player).setBounty(bounty);

                    if (bounty == 0)
                        MWBase::Environment::get().getWorld()->getPlayer().recordCrimeId();
                }
        };

        class OpModPCCrimeLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    MWWorld::Ptr player = world->getPlayerPtr();

                    player.getClass().getNpcStats(player).setBounty(static_cast<int>(runtime[0].mFloat) + player.getClass().getNpcStats(player).getBounty());
                    runtime.pop();
                }
        };

        template<class R>
        class OpAddSpell : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (id);

                    MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);
                    creatureStats.getSpells().add(id);
                    ESM::Spell::SpellType type = static_cast<ESM::Spell::SpellType>(spell->mData.mType);
                    if (type != ESM::Spell::ST_Spell && type != ESM::Spell::ST_Power)
                    {
                        // Apply looping particles immediately for constant effects
                        MWBase::Environment::get().getWorld()->applyLoopingParticles(ptr);
                    }
                }
        };

        template<class R>
        class OpRemoveSpell : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);
                    // The spell may have an instant effect which must be handled before the spell's removal.
                    for (const auto& effect : creatureStats.getSpells().getMagicEffects())
                    {
                        if (effect.second.getMagnitude() <= 0)
                            continue;
                        MWMechanics::CastSpell cast(ptr, ptr);
                        if (cast.applyInstantEffect(ptr, ptr, effect.first, effect.second.getMagnitude()))
                            creatureStats.getSpells().purgeEffect(effect.first.mId);
                    }

                    creatureStats.getSpells().remove (id);

                    MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();

                    if (ptr == MWMechanics::getPlayer() &&
                        id == wm->getSelectedSpell())
                    {
                        wm->unsetSelectedSpell();
                    }
                }
        };

        template<class R>
        class OpRemoveSpellEffects : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string spellid = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    ptr.getClass().getCreatureStats (ptr).getActiveSpells().removeEffects(spellid);
                    ptr.getClass().getCreatureStats (ptr).getSpells().removeEffects(spellid);
                }
        };

        template<class R>
        class OpRemoveEffects : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer effectId = runtime[0].mInteger;
                    runtime.pop();

                    ptr.getClass().getCreatureStats (ptr).getActiveSpells().purgeEffect(effectId);
                }
        };

        template<class R>
        class OpGetSpell : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {

                    MWWorld::Ptr ptr = R()(runtime);

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer value = 0;

                    if (ptr.getClass().isActor() && ptr.getClass().getCreatureStats(ptr).getSpells().hasSpell(id))
                        value = 1;

                    runtime.push (value);
                }
        };

        template<class R>
        class OpPCJoinFaction : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr actor = R()(runtime, false);

                    std::string factionID = "";

                    if(arg0==0)
                    {
                        factionID = getDialogueActorFaction(actor);
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    ::Misc::StringUtils::lowerCaseInPlace(factionID);
                    // Make sure this faction exists
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID);

                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWMechanics::getPlayer();
                        player.getClass().getNpcStats(player).joinFaction(factionID);
                    }
                }
        };

        template<class R>
        class OpPCRaiseRank : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr actor = R()(runtime, false);

                    std::string factionID = "";

                    if(arg0==0)
                    {
                        factionID = getDialogueActorFaction(actor);
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    ::Misc::StringUtils::lowerCaseInPlace(factionID);
                    // Make sure this faction exists
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID);

                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWMechanics::getPlayer();
                        if(player.getClass().getNpcStats(player).getFactionRanks().find(factionID) == player.getClass().getNpcStats(player).getFactionRanks().end())
                        {
                            player.getClass().getNpcStats(player).joinFaction(factionID);
                        }
                        else
                        {
                            player.getClass().getNpcStats(player).raiseRank(factionID);
                        }
                    }
                }
        };

        template<class R>
        class OpPCLowerRank : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr actor = R()(runtime, false);

                    std::string factionID = "";

                    if(arg0==0)
                    {
                        factionID = getDialogueActorFaction(actor);
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    ::Misc::StringUtils::lowerCaseInPlace(factionID);
                    // Make sure this faction exists
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID);

                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWMechanics::getPlayer();
                        player.getClass().getNpcStats(player).lowerRank(factionID);
                    }
                }
        };

        template<class R>
        class OpGetPCRank : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr ptr = R()(runtime, false);

                    std::string factionID = "";
                    if(arg0 >0)
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        factionID = ptr.getClass().getPrimaryFaction(ptr);
                    }
                    ::Misc::StringUtils::lowerCaseInPlace(factionID);
                    // Make sure this faction exists
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID);

                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    if(factionID!="")
                    {
                        if(player.getClass().getNpcStats(player).getFactionRanks().find(factionID) != player.getClass().getNpcStats(player).getFactionRanks().end())
                        {
                            runtime.push(player.getClass().getNpcStats(player).getFactionRanks().at(factionID));
                        }
                        else
                        {
                            runtime.push(-1);
                        }
                    }
                    else
                    {
                        runtime.push(-1);
                    }
                }
        };

        template<class R>
        class OpModDisposition : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    if (ptr.getClass().isNpc())
                        ptr.getClass().getNpcStats (ptr).setBaseDisposition
                            (ptr.getClass().getNpcStats (ptr).getBaseDisposition() + value);

                    // else: must not throw exception (used by an Almalexia dialogue script)
                }
        };

        template<class R>
        class OpSetDisposition : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    if (ptr.getClass().isNpc())
                        ptr.getClass().getNpcStats (ptr).setBaseDisposition (value);
                }
        };

        template<class R>
        class OpGetDisposition : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    if (!ptr.getClass().isNpc())
                        runtime.push(0);
                    else
                        runtime.push (MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(ptr));
                }
        };

        class OpGetDeadCount : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime[0].mInteger = MWBase::Environment::get().getMechanicsManager()->countDeaths (id);
                }
        };

        template<class R>
        class OpGetPCFacRep : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr ptr = R()(runtime, false);

                    std::string factionId;

                    if (arg0==1)
                    {
                        factionId = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        factionId = getDialogueActorFaction(ptr);
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    ::Misc::StringUtils::lowerCaseInPlace (factionId);

                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    runtime.push (
                        player.getClass().getNpcStats (player).getFactionReputation (factionId));
                }
        };

        template<class R>
        class OpSetPCFacRep : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr ptr = R()(runtime, false);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    std::string factionId;

                    if (arg0==1)
                    {
                        factionId = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        factionId = getDialogueActorFaction(ptr);
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    ::Misc::StringUtils::lowerCaseInPlace (factionId);

                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    player.getClass().getNpcStats (player).setFactionReputation (factionId, value);
                }
        };

        template<class R>
        class OpModPCFacRep : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr ptr = R()(runtime, false);

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    std::string factionId;

                    if (arg0==1)
                    {
                        factionId = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        factionId = getDialogueActorFaction(ptr);
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    ::Misc::StringUtils::lowerCaseInPlace (factionId);

                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    player.getClass().getNpcStats (player).setFactionReputation (factionId,
                        player.getClass().getNpcStats (player).getFactionReputation (factionId)+
                        value);
                }
        };

        template<class R>
        class OpGetCommonDisease : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push (ptr.getClass().getCreatureStats (ptr).hasCommonDisease());
                }
        };

        template<class R>
        class OpGetBlightDisease : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push (ptr.getClass().getCreatureStats (ptr).hasBlightDisease());
                }
        };

        template<class R>
        class OpGetRace : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::ConstPtr ptr = R()(runtime);

                    std::string race = runtime.getStringLiteral(runtime[0].mInteger);
                    ::Misc::StringUtils::lowerCaseInPlace(race);
                    runtime.pop();

                    std::string npcRace = ptr.get<ESM::NPC>()->mBase->mRace;
                    ::Misc::StringUtils::lowerCaseInPlace(npcRace);

                    runtime.push (npcRace == race);
            }
        };

        class OpGetWerewolfKills : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld ()->getPlayerPtr();

                    runtime.push (ptr.getClass().getNpcStats (ptr).getWerewolfKills ());
                }
        };

        template <class R>
        class OpPcExpelled : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr ptr = R()(runtime, false);

                    std::string factionID = "";
                    if(arg0 >0 )
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        factionID = ptr.getClass().getPrimaryFaction(ptr);
                    }
                    ::Misc::StringUtils::lowerCaseInPlace(factionID);
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    if(factionID!="")
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

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr ptr = R()(runtime, false);

                    std::string factionID = "";
                    if(arg0 >0 )
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        factionID = ptr.getClass().getPrimaryFaction(ptr);
                    }
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    if(factionID!="")
                    {
                        player.getClass().getNpcStats(player).expell(factionID);
                    }
                }
        };

        template <class R>
        class OpPcClearExpelled : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::ConstPtr ptr = R()(runtime, false);

                    std::string factionID = "";
                    if(arg0 >0 )
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        factionID = ptr.getClass().getPrimaryFaction(ptr);
                    }
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    if(factionID!="")
                        player.getClass().getNpcStats(player).clearExpelled(factionID);
                }
        };

        template <class R>
        class OpRaiseRank : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string factionID = ptr.getClass().getPrimaryFaction(ptr);
                    if(factionID.empty())
                        return;

                    MWWorld::Ptr player = MWMechanics::getPlayer();

                    // no-op when executed on the player
                    if (ptr == player)
                        return;

                    // If we already changed rank for this NPC, modify current rank in the NPC stats.
                    // Otherwise take rank from base NPC record, increase it and put it to NPC data.
                    int currentRank = ptr.getClass().getNpcStats(ptr).getFactionRank(factionID);
                    if (currentRank >= 0)
                        ptr.getClass().getNpcStats(ptr).raiseRank(factionID);
                    else
                    {
                        int rank = ptr.getClass().getPrimaryFactionRank(ptr);
                        rank++;
                        ptr.getClass().getNpcStats(ptr).joinFaction(factionID);
                        for (int i=0; i<rank; i++)
                            ptr.getClass().getNpcStats(ptr).raiseRank(factionID);
                    }
                }
        };

        template <class R>
        class OpLowerRank : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string factionID = ptr.getClass().getPrimaryFaction(ptr);
                    if(factionID.empty())
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
                        ptr.getClass().getNpcStats(ptr).lowerRank(factionID);
                    else
                    {
                        int rank = ptr.getClass().getPrimaryFactionRank(ptr);
                        rank--;
                        ptr.getClass().getNpcStats(ptr).joinFaction(factionID);
                        for (int i=0; i<rank; i++)
                            ptr.getClass().getNpcStats(ptr).raiseRank(factionID);
                    }
                }
        };

        template <class R>
        class OpOnDeath : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value =
                        ptr.getClass().getCreatureStats (ptr).hasDied();

                    if (value)
                        ptr.getClass().getCreatureStats (ptr).clearHasDied();

                    runtime.push (value);
                }
        };

        template <class R>
        class OpOnMurder : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value =
                        ptr.getClass().getCreatureStats (ptr).hasBeenMurdered();

                    if (value)
                        ptr.getClass().getCreatureStats (ptr).clearHasBeenMurdered();

                    runtime.push (value);
                }
        };

        template <class R>
        class OpOnKnockout : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Integer value =
                        ptr.getClass().getCreatureStats (ptr).getKnockedDownOneFrame();

                    runtime.push (value);
                }
        };

        template <class R>
        class OpIsWerewolf : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    runtime.push(ptr.getClass().getNpcStats(ptr).isWerewolf());
                }
        };

        template <class R, bool set>
        class OpSetWerewolf : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    MWBase::Environment::get().getMechanicsManager()->setWerewolf(ptr, set);
                }
        };

        template <class R>
        class OpSetWerewolfAcrobatics : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    MWBase::Environment::get().getMechanicsManager()->applyWerewolfAcrobatics(ptr);
                }
        };

        template <class R>
        class OpResurrect : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

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
                        MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);

                        // HACK: disable/enable object to re-add it to the scene properly (need a new Animation).
                        MWBase::Environment::get().getWorld()->disable(ptr);
                        // resets runtime state such as inventory, stats and AI. does not reset position in the world
                        ptr.getRefData().setCustomData(nullptr);
                        if (wasEnabled)
                            MWBase::Environment::get().getWorld()->enable(ptr);
                    }
                }
        };

        template <class R>
        class OpGetStat : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                // dummy
                runtime.push(0);
            }
        };

        template <class R>
        class OpGetMagicEffect : public Interpreter::Opcode0
        {
            int mPositiveEffect;
            int mNegativeEffect;

        public:
            OpGetMagicEffect (int positiveEffect, int negativeEffect)
                : mPositiveEffect(positiveEffect)
                , mNegativeEffect(negativeEffect)
            {
            }

            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWWorld::Ptr ptr = R()(runtime);

                const MWMechanics::MagicEffects& effects = ptr.getClass().getCreatureStats(ptr).getMagicEffects();
                float currentValue = effects.get(mPositiveEffect).getMagnitude();
                if (mNegativeEffect != -1)
                    currentValue -= effects.get(mNegativeEffect).getMagnitude();

                // GetResist* should take in account elemental shields
                if (mPositiveEffect == ESM::MagicEffect::ResistFire)
                    currentValue += effects.get(ESM::MagicEffect::FireShield).getMagnitude();
                if (mPositiveEffect == ESM::MagicEffect::ResistShock)
                    currentValue += effects.get(ESM::MagicEffect::LightningShield).getMagnitude();
                if (mPositiveEffect == ESM::MagicEffect::ResistFrost)
                    currentValue += effects.get(ESM::MagicEffect::FrostShield).getMagnitude();

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
            OpSetMagicEffect (int positiveEffect, int negativeEffect)
                : mPositiveEffect(positiveEffect)
                , mNegativeEffect(negativeEffect)
            {
            }

            virtual void execute(Interpreter::Runtime &runtime)
            {
                MWWorld::Ptr ptr = R()(runtime);
                MWMechanics::MagicEffects& effects = ptr.getClass().getCreatureStats(ptr).getMagicEffects();
                float currentValue = effects.get(mPositiveEffect).getMagnitude();
                if (mNegativeEffect != -1)
                    currentValue -= effects.get(mNegativeEffect).getMagnitude();

                // SetResist* should take in account elemental shields
                if (mPositiveEffect == ESM::MagicEffect::ResistFire)
                    currentValue += effects.get(ESM::MagicEffect::FireShield).getMagnitude();
                if (mPositiveEffect == ESM::MagicEffect::ResistShock)
                    currentValue += effects.get(ESM::MagicEffect::LightningShield).getMagnitude();
                if (mPositiveEffect == ESM::MagicEffect::ResistFrost)
                    currentValue += effects.get(ESM::MagicEffect::FrostShield).getMagnitude();

                int arg = runtime[0].mInteger;
                runtime.pop();
                effects.modifyBase(mPositiveEffect, (arg - static_cast<int>(currentValue)));
            }
        };

        template <class R>
        class OpModMagicEffect : public Interpreter::Opcode0
        {
            int mPositiveEffect;
            int mNegativeEffect;

        public:
            OpModMagicEffect (int positiveEffect, int negativeEffect)
                : mPositiveEffect(positiveEffect)
                , mNegativeEffect(negativeEffect)
            {
            }

            virtual void execute(Interpreter::Runtime &runtime)
            {
                MWWorld::Ptr ptr = R()(runtime);
                MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats(ptr);

                int arg = runtime[0].mInteger;
                runtime.pop();
                stats.getMagicEffects().modifyBase(mPositiveEffect, arg);
            }
        };

        struct MagicEffect
        {
            int mPositiveEffect;
            int mNegativeEffect;
        };

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            for (int i=0; i<Compiler::Stats::numberOfAttributes; ++i)
            {
                interpreter.installSegment5 (Compiler::Stats::opcodeGetAttribute+i, new OpGetAttribute<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeGetAttributeExplicit+i,
                    new OpGetAttribute<ExplicitRef> (i));

                interpreter.installSegment5 (Compiler::Stats::opcodeSetAttribute+i, new OpSetAttribute<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeSetAttributeExplicit+i,
                    new OpSetAttribute<ExplicitRef> (i));

                interpreter.installSegment5 (Compiler::Stats::opcodeModAttribute+i, new OpModAttribute<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeModAttributeExplicit+i,
                    new OpModAttribute<ExplicitRef> (i));
            }

            for (int i=0; i<Compiler::Stats::numberOfDynamics; ++i)
            {
                interpreter.installSegment5 (Compiler::Stats::opcodeGetDynamic+i, new OpGetDynamic<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeGetDynamicExplicit+i,
                    new OpGetDynamic<ExplicitRef> (i));

                interpreter.installSegment5 (Compiler::Stats::opcodeSetDynamic+i, new OpSetDynamic<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeSetDynamicExplicit+i,
                    new OpSetDynamic<ExplicitRef> (i));

                interpreter.installSegment5 (Compiler::Stats::opcodeModDynamic+i, new OpModDynamic<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeModDynamicExplicit+i,
                    new OpModDynamic<ExplicitRef> (i));

                interpreter.installSegment5 (Compiler::Stats::opcodeModCurrentDynamic+i,
                    new OpModCurrentDynamic<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeModCurrentDynamicExplicit+i,
                    new OpModCurrentDynamic<ExplicitRef> (i));

                interpreter.installSegment5 (Compiler::Stats::opcodeGetDynamicGetRatio+i,
                    new OpGetDynamicGetRatio<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeGetDynamicGetRatioExplicit+i,
                    new OpGetDynamicGetRatio<ExplicitRef> (i));
            }

            for (int i=0; i<Compiler::Stats::numberOfSkills; ++i)
            {
                interpreter.installSegment5 (Compiler::Stats::opcodeGetSkill+i, new OpGetSkill<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeGetSkillExplicit+i, new OpGetSkill<ExplicitRef> (i));

                interpreter.installSegment5 (Compiler::Stats::opcodeSetSkill+i, new OpSetSkill<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeSetSkillExplicit+i, new OpSetSkill<ExplicitRef> (i));

                interpreter.installSegment5 (Compiler::Stats::opcodeModSkill+i, new OpModSkill<ImplicitRef> (i));
                interpreter.installSegment5 (Compiler::Stats::opcodeModSkillExplicit+i, new OpModSkill<ExplicitRef> (i));
            }

            interpreter.installSegment5 (Compiler::Stats::opcodeGetPCCrimeLevel, new OpGetPCCrimeLevel);
            interpreter.installSegment5 (Compiler::Stats::opcodeSetPCCrimeLevel, new OpSetPCCrimeLevel);
            interpreter.installSegment5 (Compiler::Stats::opcodeModPCCrimeLevel, new OpModPCCrimeLevel);

            interpreter.installSegment5 (Compiler::Stats::opcodeAddSpell, new OpAddSpell<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeAddSpellExplicit, new OpAddSpell<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeRemoveSpell, new OpRemoveSpell<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeRemoveSpellExplicit,
                new OpRemoveSpell<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeRemoveSpellEffects, new OpRemoveSpellEffects<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeRemoveSpellEffectsExplicit,
                new OpRemoveSpellEffects<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeResurrect, new OpResurrect<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeResurrectExplicit,
                new OpResurrect<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeRemoveEffects, new OpRemoveEffects<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeRemoveEffectsExplicit,
                new OpRemoveEffects<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeGetSpell, new OpGetSpell<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetSpellExplicit, new OpGetSpell<ExplicitRef>);

            interpreter.installSegment3(Compiler::Stats::opcodePCRaiseRank,new OpPCRaiseRank<ImplicitRef>);
            interpreter.installSegment3(Compiler::Stats::opcodePCLowerRank,new OpPCLowerRank<ImplicitRef>);
            interpreter.installSegment3(Compiler::Stats::opcodePCJoinFaction,new OpPCJoinFaction<ImplicitRef>);
            interpreter.installSegment3(Compiler::Stats::opcodePCRaiseRankExplicit,new OpPCRaiseRank<ExplicitRef>);
            interpreter.installSegment3(Compiler::Stats::opcodePCLowerRankExplicit,new OpPCLowerRank<ExplicitRef>);
            interpreter.installSegment3(Compiler::Stats::opcodePCJoinFactionExplicit,new OpPCJoinFaction<ExplicitRef>);
            interpreter.installSegment3(Compiler::Stats::opcodeGetPCRank,new OpGetPCRank<ImplicitRef>);
            interpreter.installSegment3(Compiler::Stats::opcodeGetPCRankExplicit,new OpGetPCRank<ExplicitRef>);

            interpreter.installSegment5(Compiler::Stats::opcodeModDisposition,new OpModDisposition<ImplicitRef>);
            interpreter.installSegment5(Compiler::Stats::opcodeModDispositionExplicit,new OpModDisposition<ExplicitRef>);
            interpreter.installSegment5(Compiler::Stats::opcodeSetDisposition,new OpSetDisposition<ImplicitRef>);
            interpreter.installSegment5(Compiler::Stats::opcodeSetDispositionExplicit,new OpSetDisposition<ExplicitRef>);
            interpreter.installSegment5(Compiler::Stats::opcodeGetDisposition,new OpGetDisposition<ImplicitRef>);
            interpreter.installSegment5(Compiler::Stats::opcodeGetDispositionExplicit,new OpGetDisposition<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeGetLevel, new OpGetLevel<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetLevelExplicit, new OpGetLevel<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeSetLevel, new OpSetLevel<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeSetLevelExplicit, new OpSetLevel<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeGetDeadCount, new OpGetDeadCount);

            interpreter.installSegment3 (Compiler::Stats::opcodeGetPCFacRep, new OpGetPCFacRep<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodeGetPCFacRepExplicit, new OpGetPCFacRep<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodeSetPCFacRep, new OpSetPCFacRep<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodeSetPCFacRepExplicit, new OpSetPCFacRep<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodeModPCFacRep, new OpModPCFacRep<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodeModPCFacRepExplicit, new OpModPCFacRep<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeGetCommonDisease, new OpGetCommonDisease<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetCommonDiseaseExplicit, new OpGetCommonDisease<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetBlightDisease, new OpGetBlightDisease<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetBlightDiseaseExplicit, new OpGetBlightDisease<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeGetRace, new OpGetRace<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetRaceExplicit, new OpGetRace<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetWerewolfKills, new OpGetWerewolfKills);

            interpreter.installSegment3 (Compiler::Stats::opcodePcExpelled, new OpPcExpelled<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodePcExpelledExplicit, new OpPcExpelled<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodePcExpell, new OpPcExpell<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodePcExpellExplicit, new OpPcExpell<ExplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodePcClearExpelled, new OpPcClearExpelled<ImplicitRef>);
            interpreter.installSegment3 (Compiler::Stats::opcodePcClearExpelledExplicit, new OpPcClearExpelled<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeRaiseRank, new OpRaiseRank<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeRaiseRankExplicit, new OpRaiseRank<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeLowerRank, new OpLowerRank<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeLowerRankExplicit, new OpLowerRank<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeOnDeath, new OpOnDeath<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeOnDeathExplicit, new OpOnDeath<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeOnMurder, new OpOnMurder<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeOnMurderExplicit, new OpOnMurder<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeOnKnockout, new OpOnKnockout<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeOnKnockoutExplicit, new OpOnKnockout<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeIsWerewolf, new OpIsWerewolf<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeIsWerewolfExplicit, new OpIsWerewolf<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeBecomeWerewolf, new OpSetWerewolf<ImplicitRef, true>);
            interpreter.installSegment5 (Compiler::Stats::opcodeBecomeWerewolfExplicit, new OpSetWerewolf<ExplicitRef, true>);
            interpreter.installSegment5 (Compiler::Stats::opcodeUndoWerewolf, new OpSetWerewolf<ImplicitRef, false>);
            interpreter.installSegment5 (Compiler::Stats::opcodeUndoWerewolfExplicit, new OpSetWerewolf<ExplicitRef, false>);
            interpreter.installSegment5 (Compiler::Stats::opcodeSetWerewolfAcrobatics, new OpSetWerewolfAcrobatics<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeSetWerewolfAcrobaticsExplicit, new OpSetWerewolfAcrobatics<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetStat, new OpGetStat<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetStatExplicit, new OpGetStat<ExplicitRef>);

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

            for (int i=0; i<24; ++i)
            {
                int positive = sMagicEffects[i].mPositiveEffect;
                int negative = sMagicEffects[i].mNegativeEffect;

                interpreter.installSegment5 (Compiler::Stats::opcodeGetMagicEffect+i, new OpGetMagicEffect<ImplicitRef> (positive, negative));
                interpreter.installSegment5 (Compiler::Stats::opcodeGetMagicEffectExplicit+i, new OpGetMagicEffect<ExplicitRef> (positive, negative));

                interpreter.installSegment5 (Compiler::Stats::opcodeSetMagicEffect+i, new OpSetMagicEffect<ImplicitRef> (positive, negative));
                interpreter.installSegment5 (Compiler::Stats::opcodeSetMagicEffectExplicit+i, new OpSetMagicEffect<ExplicitRef> (positive, negative));

                interpreter.installSegment5 (Compiler::Stats::opcodeModMagicEffect+i, new OpModMagicEffect<ImplicitRef> (positive, negative));
                interpreter.installSegment5 (Compiler::Stats::opcodeModMagicEffectExplicit+i, new OpModMagicEffect<ExplicitRef> (positive, negative));
            }
        }
    }
}
