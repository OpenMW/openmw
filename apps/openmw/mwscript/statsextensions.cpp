
#include "statsextensions.hpp"

#include <cmath>

#include <boost/algorithm/string.hpp>

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

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace
{
    std::string getDialogueActorFaction()
    {
        MWWorld::Ptr actor = MWBase::Environment::get().getDialogueManager()->getActor();

        const MWMechanics::NpcStats &stats = MWWorld::Class::get (actor).getNpcStats (actor);

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
                        MWWorld::Class::get (ptr)
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

                    MWWorld::Class::get (ptr)
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
                        MWWorld::Class::get (ptr)
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

                    MWMechanics::Stat<int> attribute = ptr.getClass().getCreatureStats(ptr).getAttribute(mIndex);
                    attribute.setModified (value, 0);
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

                    MWMechanics::Stat<int> attribute = MWWorld::Class::get(ptr)
                        .getCreatureStats(ptr)
                        .getAttribute(mIndex);

                    value +=
                            attribute.getModified();

                    attribute
                        .setModified (value, 0, 100);
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

                    if (mIndex==0 && MWWorld::Class::get (ptr).hasItemHealth (ptr))
                    {
                        // health is a special case
                        value = MWWorld::Class::get (ptr).getItemMaxHealth (ptr);
                    } else {
                        value =
                            MWWorld::Class::get(ptr)
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

                    MWMechanics::DynamicStat<float> stat (MWWorld::Class::get (ptr).getCreatureStats (ptr)
                        .getDynamic (mIndex));

                    stat.setModified (value, 0);
                    stat.setCurrent(value);

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setDynamic (mIndex, stat);
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

                    MWMechanics::CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Float current = stats.getDynamic(mIndex).getCurrent();

                    MWMechanics::DynamicStat<float> stat (MWWorld::Class::get (ptr).getCreatureStats (ptr)
                        .getDynamic (mIndex));

                    stat.setModified (diff + stat.getModified(), 0);

                    stat.setCurrent (diff + current);

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setDynamic (mIndex, stat);
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

                    MWMechanics::CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Float current = stats.getDynamic(mIndex).getCurrent();

                    MWMechanics::DynamicStat<float> stat (MWWorld::Class::get (ptr).getCreatureStats (ptr)
                        .getDynamic (mIndex));

                    stat.setCurrent (diff + current);

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).setDynamic (mIndex, stat);
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

                    MWMechanics::CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

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

                    Interpreter::Type_Integer value =
                        MWWorld::Class::get (ptr).getNpcStats (ptr).getSkill (mIndex).
                        getModified();

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

                    MWMechanics::NpcStats& stats = MWWorld::Class::get (ptr).getNpcStats (ptr);

                    MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

                    assert (ref);

                    const ESM::Class& class_ =
                        *MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find (ref->mBase->mClass);

                    float level = 0;
                    float progress = std::modf (stats.getSkill (mIndex).getBase(), &level);

                    float modifier = stats.getSkill (mIndex).getModifier();

                    int newLevel = static_cast<int> (value-modifier);

                    if (newLevel<0)
                        newLevel = 0;
                    else if (newLevel>100)
                        newLevel = 100;

                    progress = (progress / stats.getSkillGain (mIndex, class_, -1, level))
                        * stats.getSkillGain (mIndex, class_, -1, newLevel);

                    if (progress>=1)
                        progress = 0.999999999;

                    stats.getSkill (mIndex).set (newLevel + progress);
                    stats.getSkill (mIndex).setModifier (modifier);
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

                    value += MWWorld::Class::get (ptr).getNpcStats (ptr).getSkill (mIndex).
                        getModified();

                    MWWorld::Class::get (ptr).getNpcStats (ptr).getSkill (mIndex).
                        setModified (value, 0, 100);
                }
        };

        class OpGetPCCrimeLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    MWWorld::Ptr player = world->getPlayer().getPlayer();
                    runtime.push (static_cast <Interpreter::Type_Float> (MWWorld::Class::get (player).getNpcStats (player).getBounty()));
                }
        };

        class OpSetPCCrimeLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    MWWorld::Ptr player = world->getPlayer().getPlayer();

                    MWWorld::Class::get (player).getNpcStats (player).setBounty(runtime[0].mFloat);
                    runtime.pop();
                }
        };

        class OpModPCCrimeLevel : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::World *world = MWBase::Environment::get().getWorld();
                    MWWorld::Ptr player = world->getPlayer().getPlayer();

                    MWWorld::Class::get (player).getNpcStats (player).setBounty(runtime[0].mFloat + MWWorld::Class::get (player).getNpcStats (player).getBounty());
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

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).getSpells().add (id);
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

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).getSpells().remove (id);
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
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).getSpells().begin());
                        iter!=MWWorld::Class::get (ptr).getCreatureStats (ptr).getSpells().end(); ++iter)
                        if (iter->first==id)
                        {
                            value = 1;
                            break;
                        }

                    runtime.push (value);
                }
        };

        class OpPCJoinFaction : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    std::string factionID = "";

                    if(arg0==0)
                    {
                        factionID = getDialogueActorFaction();
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    Misc::StringUtils::toLower(factionID);
                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                        if(MWWorld::Class::get(player).getNpcStats(player).getFactionRanks().find(factionID) == MWWorld::Class::get(player).getNpcStats(player).getFactionRanks().end())
                        {
                            MWWorld::Class::get(player).getNpcStats(player).getFactionRanks()[factionID] = 0;
                        }
                    }
                }
        };

        class OpPCRaiseRank : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    std::string factionID = "";

                    if(arg0==0)
                    {
                        factionID = getDialogueActorFaction();
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    Misc::StringUtils::toLower(factionID);
                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                        if(MWWorld::Class::get(player).getNpcStats(player).getFactionRanks().find(factionID) == MWWorld::Class::get(player).getNpcStats(player).getFactionRanks().end())
                        {
                            MWWorld::Class::get(player).getNpcStats(player).getFactionRanks()[factionID] = 0;
                        }
                        else
                        {
                            MWWorld::Class::get(player).getNpcStats(player).getFactionRanks()[factionID] = MWWorld::Class::get(player).getNpcStats(player).getFactionRanks()[factionID] +1;
                        }
                    }
                }
        };

        class OpPCLowerRank : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    std::string factionID = "";

                    if(arg0==0)
                    {
                        factionID = getDialogueActorFaction();
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    Misc::StringUtils::toLower(factionID);
                    if(factionID != "")
                    {
                        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                        if(MWWorld::Class::get(player).getNpcStats(player).getFactionRanks().find(factionID) != MWWorld::Class::get(player).getNpcStats(player).getFactionRanks().end())
                        {
                            MWWorld::Class::get(player).getNpcStats(player).getFactionRanks()[factionID] = MWWorld::Class::get(player).getNpcStats(player).getFactionRanks()[factionID] -1;
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
                        if(MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().empty())
                        {
                            factionID = "";
                        }
                        else
                        {
                            factionID = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    Misc::StringUtils::toLower(factionID);
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                    if(factionID!="")
                    {
                        if(MWWorld::Class::get(player).getNpcStats(player).getFactionRanks().find(factionID) != MWWorld::Class::get(player).getNpcStats(player).getFactionRanks().end())
                        {
                            runtime.push(MWWorld::Class::get(player).getNpcStats(player).getFactionRanks()[factionID]);
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

                    MWWorld::Class::get (ptr).getNpcStats (ptr).setBaseDisposition
                        (MWWorld::Class::get (ptr).getNpcStats (ptr).getBaseDisposition() + value);
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

                    MWWorld::Class::get (ptr).getNpcStats (ptr).setBaseDisposition (value);
                }
        };

        template<class R>
        class OpGetDisposition : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

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

                        if (!MWWorld::Class::get (ptr).getNpcStats (ptr).getFactionRanks().empty())
                            factionId = MWWorld::Class::get (ptr).getNpcStats (ptr).getFactionRanks().begin()->first;
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    Misc::StringUtils::toLower (factionId);

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                    runtime.push (
                        MWWorld::Class::get (player).getNpcStats (player).getFactionReputation (factionId));
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

                        if (!MWWorld::Class::get (ptr).getNpcStats (ptr).getFactionRanks().empty())
                            factionId = MWWorld::Class::get (ptr).getNpcStats (ptr).getFactionRanks().begin()->first;
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    Misc::StringUtils::toLower (factionId);

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                    MWWorld::Class::get (player).getNpcStats (player).setFactionReputation (factionId, value);
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

                        if (!MWWorld::Class::get (ptr).getNpcStats (ptr).getFactionRanks().empty())
                            factionId = MWWorld::Class::get (ptr).getNpcStats (ptr).getFactionRanks().begin()->first;
                    }

                    if (factionId.empty())
                        throw std::runtime_error ("failed to determine faction");

                    Misc::StringUtils::toLower (factionId);

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                    MWWorld::Class::get (player).getNpcStats (player).setFactionReputation (factionId,
                        MWWorld::Class::get (player).getNpcStats (player).getFactionReputation (factionId)+
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

                    runtime.push (MWWorld::Class::get (ptr).getCreatureStats (ptr).hasCommonDisease());
                }
        };

        template<class R>
        class OpGetBlightDisease : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push (MWWorld::Class::get (ptr).getCreatureStats (ptr).hasBlightDisease());
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
                    Misc::StringUtils::toLower(race);
                    runtime.pop();

                    std::string npcRace = ptr.get<ESM::NPC>()->mBase->mRace;
                    Misc::StringUtils::toLower(npcRace);

                    runtime.push (npcRace == race);
            }
        };

        class OpGetWerewolfKills : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ();

                    runtime.push (MWWorld::Class::get(ptr).getNpcStats (ptr).getWerewolfKills ());
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
                        if(MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().empty())
                        {
                            factionID = "";
                        }
                        else
                        {
                            factionID = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    Misc::StringUtils::toLower(factionID);
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                    if(factionID!="")
                    {
                        std::set<std::string>& expelled = MWWorld::Class::get(player).getNpcStats(player).getExpelled ();
                        if (expelled.find (factionID) != expelled.end())
                        {
                            runtime.push(1);
                        }
                        else
                        {
                            runtime.push(0);
                        }
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
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string factionID = "";
                    if(arg0 >0 )
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    else
                    {
                        if(MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().empty())
                        {
                            factionID = "";
                        }
                        else
                        {
                            factionID = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                    if(factionID!="")
                    {
                        std::set<std::string>& expelled = MWWorld::Class::get(player).getNpcStats(player).getExpelled ();
                        Misc::StringUtils::toLower(factionID);
                        expelled.insert(factionID);
                    }
                }
        };

        template <class R>
        class OpPcClearExpelled : public Interpreter::Opcode1
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
                        if(MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().empty())
                        {
                            factionID = "";
                        }
                        else
                        {
                            factionID = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                    if(factionID!="")
                    {
                        std::set<std::string>& expelled = MWWorld::Class::get(player).getNpcStats(player).getExpelled ();
                        Misc::StringUtils::toLower(factionID);
                        expelled.erase (factionID);
                    }
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
                    if(MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().empty())
                        return;
                    else
                    {
                        factionID = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().begin()->first;
                    }
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

                    // no-op when executed on the player
                    if (ptr == player)
                        return;

                    std::map<std::string, int>& ranks = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks ();
                    ranks[factionID] = ranks[factionID]+1;
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
                    if(MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().empty())
                        return;
                    else
                    {
                        factionID = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().begin()->first;
                    }
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

                    // no-op when executed on the player
                    if (ptr == player)
                        return;

                    std::map<std::string, int>& ranks = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks ();
                    ranks[factionID] = ranks[factionID]-1;
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
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).hasDied();

                    if (value)
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).clearHasDied();

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
                    runtime.push(MWWorld::Class::get(ptr).getNpcStats(ptr).isWerewolf());
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

            interpreter.installSegment5 (Compiler::Stats::opcodeGetSpell, new OpGetSpell<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeGetSpellExplicit, new OpGetSpell<ExplicitRef>);

            interpreter.installSegment3(Compiler::Stats::opcodePCRaiseRank,new OpPCRaiseRank);
            interpreter.installSegment3(Compiler::Stats::opcodePCLowerRank,new OpPCLowerRank);
            interpreter.installSegment3(Compiler::Stats::opcodePCJoinFaction,new OpPCJoinFaction);
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

            interpreter.installSegment5 (Compiler::Stats::opcodeIsWerewolf, new OpIsWerewolf<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeIsWerewolfExplicit, new OpIsWerewolf<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Stats::opcodeBecomeWerewolf, new OpSetWerewolf<ImplicitRef, true>);
            interpreter.installSegment5 (Compiler::Stats::opcodeBecomeWerewolfExplicit, new OpSetWerewolf<ExplicitRef, true>);
            interpreter.installSegment5 (Compiler::Stats::opcodeUndoWerewolf, new OpSetWerewolf<ImplicitRef, false>);
            interpreter.installSegment5 (Compiler::Stats::opcodeUndoWerewolfExplicit, new OpSetWerewolf<ExplicitRef, false>);
            interpreter.installSegment5 (Compiler::Stats::opcodeSetWerewolfAcrobatics, new OpSetWerewolfAcrobatics<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Stats::opcodeSetWerewolfAcrobaticsExplicit, new OpSetWerewolfAcrobatics<ExplicitRef>);           
        }
    }
}
