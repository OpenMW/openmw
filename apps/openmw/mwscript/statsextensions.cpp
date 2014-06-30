
#include "statsextensions.hpp"

#include <cmath>

#include <components/esm/loadnpc.hpp>

#include "../mwworld/esmstore.hpp"

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace
{
    std::string getDialogueActorFaction(MWWorld::Ptr actor)
    {
        const MWMechanics::NpcStats &stats = actor.getClass().getNpcStats (actor);

        if (stats.getFactionRanks().empty())
            throw std::runtime_error (
                "failed to determine dialogue actors faction (because actor is factionless)");

        return stats.getFactionRanks().begin()->first;
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
                    attribute.setBase (value - (attribute.getModified() - attribute.getBase()));
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

                    attribute.setBase (std::min(100, attribute.getBase() + value));
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
                        value = ptr.getClass().getItemMaxHealth (ptr);
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
                    MWWorld::Ptr ptr = R()(runtime);

                    Interpreter::Type_Float diff = runtime[0].mFloat;
                    runtime.pop();

                    MWMechanics::CreatureStats& stats = ptr.getClass().getCreatureStats (ptr);

                    Interpreter::Type_Float current = stats.getDynamic(mIndex).getCurrent();

                    MWMechanics::DynamicStat<float> stat (ptr.getClass().getCreatureStats (ptr)
                        .getDynamic (mIndex));

                    stat.setModified (diff + stat.getModified(), 0);

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

                    stat.setCurrent (diff + current, true);

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

                    MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

                    assert (ref);

                    const ESM::Class& class_ =
                        *MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find (ref->mBase->mClass);

                    float level = stats.getSkill(mIndex).getBase();
                    float progress = stats.getSkill(mIndex).getProgress();

                    int newLevel = value - (stats.getSkill(mIndex).getModified() - stats.getSkill(mIndex).getBase());

                    if (newLevel<0)
                        newLevel = 0;

                    progress = (progress / stats.getSkillGain (mIndex, class_, -1, level))
                        * stats.getSkillGain (mIndex, class_, -1, newLevel);

                    if (progress>=1)
                        progress = 0.999999999;

                    stats.getSkill (mIndex).setBase (newLevel);
                    stats.getSkill (mIndex).setProgress(progress);
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

                    MWMechanics::NpcStats& stats = ptr.getClass().getNpcStats(ptr);

                    stats.getSkill(mIndex).
                        setBase (std::min(100, stats.getSkill(mIndex).getBase() + value));
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

                    player.getClass().getNpcStats (player).setBounty(runtime[0].mFloat);
                    runtime.pop();
                }
        };

        class OpModPCCrimeLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    MWWorld::Ptr player = world->getPlayerPtr();

                    player.getClass().getNpcStats (player).setBounty(runtime[0].mFloat + player.getClass().getNpcStats (player).getBounty());
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

                    // make sure a spell with this ID actually exists.
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (id);

                    ptr.getClass().getCreatureStats (ptr).getSpells().add (id);
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

                    ptr.getClass().getCreatureStats (ptr).getSpells().remove (id);

                    MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();

                    if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr() &&
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

                    for (MWMechanics::Spells::TIterator iter (
                        ptr.getClass().getCreatureStats (ptr).getSpells().begin());
                        iter!=ptr.getClass().getCreatureStats (ptr).getSpells().end(); ++iter)
                        if (iter->first==id)
                        {
                            value = 1;
                            break;
                        }

                    runtime.push (value);
                }
        };

        template<class R>
        class OpPCJoinFaction : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    std::string factionID = "";

                    if(arg0==0)
                    {
                        MWWorld::Ptr actor = R()(runtime);
                        factionID = getDialogueActorFaction(actor);
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    ::Misc::StringUtils::toLower(factionID);
                    // Make sure this faction exists
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID);

                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                        if(player.getClass().getNpcStats(player).getFactionRanks().find(factionID) == player.getClass().getNpcStats(player).getFactionRanks().end())
                        {
                            player.getClass().getNpcStats(player).getFactionRanks()[factionID] = 0;
                        }
                    }
                }
        };

        template<class R>
        class OpPCRaiseRank : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    std::string factionID = "";

                    if(arg0==0)
                    {
                        MWWorld::Ptr actor = R()(runtime);
                        factionID = getDialogueActorFaction(actor);
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    ::Misc::StringUtils::toLower(factionID);
                    // Make sure this faction exists
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID);

                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                        if(player.getClass().getNpcStats(player).getFactionRanks().find(factionID) == player.getClass().getNpcStats(player).getFactionRanks().end())
                        {
                            player.getClass().getNpcStats(player).getFactionRanks()[factionID] = 0;
                        }
                        else
                        {
                            player.getClass().getNpcStats(player).getFactionRanks()[factionID] =
                                    std::min(player.getClass().getNpcStats(player).getFactionRanks()[factionID] +1,
                                             9);
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
                    std::string factionID = "";

                    if(arg0==0)
                    {
                        MWWorld::Ptr actor = R()(runtime);
                        factionID = getDialogueActorFaction(actor);
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    ::Misc::StringUtils::toLower(factionID);
                    // Make sure this faction exists
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID);

                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                        if(player.getClass().getNpcStats(player).getFactionRanks().find(factionID) != player.getClass().getNpcStats(player).getFactionRanks().end())
                        {
                            player.getClass().getNpcStats(player).getFactionRanks()[factionID] =
                                    std::max(0, player.getClass().getNpcStats(player).getFactionRanks()[factionID]-1);
                        }
                    }
                }
        };

        template<class R>
        class OpGetPCRank : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string factionID = "";
                    if(arg0 >0)
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        if(ptr.getClass().getNpcStats(ptr).getFactionRanks().empty())
                        {
                            factionID = "";
                        }
                        else
                        {
                            factionID = ptr.getClass().getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    ::Misc::StringUtils::toLower(factionID);
                    // Make sure this faction exists
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(factionID);

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                    if(factionID!="")
                    {
                        if(player.getClass().getNpcStats(player).getFactionRanks().find(factionID) != player.getClass().getNpcStats(player).getFactionRanks().end())
                        {
                            runtime.push(player.getClass().getNpcStats(player).getFactionRanks()[factionID]);
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
                    std::string factionId;

                    if (arg0==1)
                    {
                        factionId = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        MWWorld::Ptr ptr = R()(runtime);

                        if (!ptr.getClass().getNpcStats (ptr).getFactionRanks().empty())
                            factionId = ptr.getClass().getNpcStats (ptr).getFactionRanks().begin()->first;
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    ::Misc::StringUtils::toLower (factionId);

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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
                        MWWorld::Ptr ptr = R()(runtime);

                        if (!ptr.getClass().getNpcStats (ptr).getFactionRanks().empty())
                            factionId = ptr.getClass().getNpcStats (ptr).getFactionRanks().begin()->first;
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    ::Misc::StringUtils::toLower (factionId);

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                    player.getClass().getNpcStats (player).setFactionReputation (factionId, value);
                }
        };

        template<class R>
        class OpModPCFacRep : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
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
                        MWWorld::Ptr ptr = R()(runtime);

                        if (!ptr.getClass().getNpcStats (ptr).getFactionRanks().empty())
                            factionId = ptr.getClass().getNpcStats (ptr).getFactionRanks().begin()->first;
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    ::Misc::StringUtils::toLower (factionId);

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string race = runtime.getStringLiteral(runtime[0].mInteger);
                    ::Misc::StringUtils::toLower(race);
                    runtime.pop();

                    std::string npcRace = ptr.get<ESM::NPC>()->mBase->mRace;
                    ::Misc::StringUtils::toLower(npcRace);

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
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string factionID = "";
                    if(arg0 >0 )
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        if(ptr.getClass().getNpcStats(ptr).getFactionRanks().empty())
                        {
                            factionID = "";
                        }
                        else
                        {
                            factionID = ptr.getClass().getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    ::Misc::StringUtils::toLower(factionID);
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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
                    std::string factionID = "";
                    if(arg0 >0 )
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        MWWorld::Ptr ptr = R()(runtime);
                        if(ptr.getClass().getNpcStats(ptr).getFactionRanks().empty())
                        {
                            factionID = "";
                        }
                        else
                        {
                            factionID = ptr.getClass().getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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
                    std::string factionID = "";
                    if(arg0 >0 )
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        MWWorld::Ptr ptr = R()(runtime);
                        if(ptr.getClass().getNpcStats(ptr).getFactionRanks().empty())
                        {
                            factionID = "";
                        }
                        else
                        {
                            factionID = ptr.getClass().getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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

                    std::string factionID = "";
                    if(ptr.getClass().getNpcStats(ptr).getFactionRanks().empty())
                        return;
                    else
                    {
                        factionID = ptr.getClass().getNpcStats(ptr).getFactionRanks().begin()->first;
                    }
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

                    // no-op when executed on the player
                    if (ptr == player)
                        return;

                    std::map<std::string, int>& ranks = ptr.getClass().getNpcStats(ptr).getFactionRanks ();
                    ranks[factionID] = std::min(9, ranks[factionID]+1);
                }
        };

        template <class R>
        class OpLowerRank : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string factionID = "";
                    if(ptr.getClass().getNpcStats(ptr).getFactionRanks().empty())
                        return;
                    else
                    {
                        factionID = ptr.getClass().getNpcStats(ptr).getFactionRanks().begin()->first;
                    }
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

                    // no-op when executed on the player
                    if (ptr == player)
                        return;

                    std::map<std::string, int>& ranks = ptr.getClass().getNpcStats(ptr).getFactionRanks ();
                    ranks[factionID] = std::max(0, ranks[factionID]-1);
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
                    MWBase::Environment::get().getWorld()->setWerewolf(ptr, set);
                }
        };

        template <class R>
        class OpSetWerewolfAcrobatics : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    MWBase::Environment::get().getWorld()->applyWerewolfAcrobatics(ptr);
                }
        };

        template <class R>
        class OpResurrect : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    ptr.getClass().getCreatureStats(ptr).resurrect();
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
        }
    }
}
