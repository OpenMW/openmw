
#include "statsextensions.hpp"

#include <cmath>

#include <boost/algorithm/string.hpp>

#include <components/esm/loadnpc.hpp>

#include <components/esm_store/store.hpp>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"


namespace MWScript
{
    namespace Stats
    {
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

                    MWWorld::Class::get(ptr)
                        .getCreatureStats(ptr)
                        .getAttribute(mIndex)
                        .setModified (value, 0);
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

                    value +=
                        MWWorld::Class::get(ptr)
                            .getCreatureStats(ptr)
                            .getAttribute(mIndex)
                            .getModified();

                    MWWorld::Class::get(ptr)
                        .getCreatureStats(ptr)
                        .getAttribute(mIndex)
                        .setModified (value, 0, 100);
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
                    Interpreter::Type_Integer value;

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

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get(ptr)
                        .getCreatureStats(ptr)
                        .getDynamic(mIndex)
                        .setModified(value, 0);
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

                    Interpreter::Type_Integer diff = runtime[0].mInteger;
                    runtime.pop();

                    MWMechanics::CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Integer current = stats.getDynamic(mIndex).getCurrent();

                    stats.getDynamic(mIndex).setModified(
                        diff + stats.getDynamic(mIndex).getModified(), 0);

                    stats.getDynamic(mIndex).setCurrent(diff + current);
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

                    Interpreter::Type_Integer diff = runtime[0].mInteger;
                    runtime.pop();

                    MWMechanics::CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Integer current = stats.getDynamic(mIndex).getCurrent();

                    stats.getDynamic(mIndex).setCurrent (diff + current);
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
                        *MWBase::Environment::get().getWorld()->getStore().classes.find (ref->base->cls);

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
                    MWBase::Environment::get().getWorld()->getStore().spells.find (id);

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
                        if (*iter==id)
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
                        factionID = MWBase::Environment::get().getDialogueManager()->getFaction();
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    boost::algorithm::to_lower(factionID);
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
                        factionID = MWBase::Environment::get().getDialogueManager()->getFaction();
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    boost::algorithm::to_lower(factionID);
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
                        factionID = MWBase::Environment::get().getDialogueManager()->getFaction();
                    }
                    else
                    {
                        factionID = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                    }
                    boost::algorithm::to_lower(factionID);
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
                            //throw exception?
                        }
                        else
                        {
                            factionID = MWWorld::Class::get(ptr).getNpcStats(ptr).getFactionRanks().begin()->first;
                        }
                    }
                    boost::algorithm::to_lower(factionID);
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

//                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    /// \todo modify disposition towards the player
                }
        };


        const int numberOfAttributes = 8;

        const int opcodeGetAttribute = 0x2000027;
        const int opcodeGetAttributeExplicit = 0x200002f;
        const int opcodeSetAttribute = 0x2000037;
        const int opcodeSetAttributeExplicit = 0x200003f;
        const int opcodeModAttribute = 0x2000047;
        const int opcodeModAttributeExplicit = 0x200004f;

        const int numberOfDynamics = 3;

        const int opcodeGetDynamic = 0x2000057;
        const int opcodeGetDynamicExplicit = 0x200005a;
        const int opcodeSetDynamic = 0x200005d;
        const int opcodeSetDynamicExplicit = 0x2000060;
        const int opcodeModDynamic = 0x2000063;
        const int opcodeModDynamicExplicit = 0x2000066;
        const int opcodeModCurrentDynamic = 0x2000069;
        const int opcodeModCurrentDynamicExplicit = 0x200006c;
        const int opcodeGetDynamicGetRatio = 0x200006f;
        const int opcodeGetDynamicGetRatioExplicit = 0x2000072;

        const int numberOfSkills = 27;

        const int opcodeGetSkill = 0x200008e;
        const int opcodeGetSkillExplicit = 0x20000a9;
        const int opcodeSetSkill = 0x20000c4;
        const int opcodeSetSkillExplicit = 0x20000df;
        const int opcodeModSkill = 0x20000fa;
        const int opcodeModSkillExplicit = 0x2000115;

        const int opcodeAddSpell = 0x2000147;
        const int opcodeAddSpellExplicit = 0x2000148;
        const int opcodeRemoveSpell = 0x2000149;
        const int opcodeRemoveSpellExplicit = 0x200014a;
        const int opcodeGetSpell = 0x200014b;
        const int opcodeGetSpellExplicit = 0x200014c;

        const int opcodePCRaiseRank = 0x2000b;
        const int opcodePCLowerRank = 0x2000c;
        const int opcodePCJoinFaction = 0x2000d;
        const int opcodeGetPCRank = 0x2000e;
        const int opcodeGetPCRankExplicit = 0x2000f;
        const int opcodeModDisposition = 0x200014d;
        const int opcodeModDispositionExplicit = 0x200014e;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            static const char *attributes[numberOfAttributes] =
            {
                "strength", "intelligence", "willpower", "agility", "speed", "endurance",
                "personality", "luck"
            };

            static const char *dynamics[numberOfDynamics] =
            {
                "health", "magicka", "fatigue"
            };

            static const char *skills[numberOfSkills] =
            {
                "block", "armorer", "mediumarmor", "heavyarmor", "bluntweapon",
                "longblade", "axe", "spear", "athletics", "enchant", "destruction",
                "alteration", "illusion", "conjuration", "mysticism",
                "restoration", "alchemy", "unarmored", "security", "sneak",
                "acrobatics", "lightarmor", "shortblade", "marksman",
                "merchantile", "speechcraft", "handtohand"
            };

            std::string get ("get");
            std::string set ("set");
            std::string mod ("mod");
            std::string modCurrent ("modcurrent");
            std::string getRatio ("getratio");

            for (int i=0; i<numberOfAttributes; ++i)
            {
                extensions.registerFunction (get + attributes[i], 'l', "",
                    opcodeGetAttribute+i, opcodeGetAttributeExplicit+i);

                extensions.registerInstruction (set + attributes[i], "l",
                    opcodeSetAttribute+i, opcodeSetAttributeExplicit+i);

                extensions.registerInstruction (mod + attributes[i], "l",
                    opcodeModAttribute+i, opcodeModAttributeExplicit+i);
            }

            for (int i=0; i<numberOfDynamics; ++i)
            {
                extensions.registerFunction (get + dynamics[i], 'l', "",
                    opcodeGetDynamic+i, opcodeGetDynamicExplicit+i);

                extensions.registerInstruction (set + dynamics[i], "l",
                    opcodeSetDynamic+i, opcodeSetDynamicExplicit+i);

                extensions.registerInstruction (mod + dynamics[i], "l",
                    opcodeModDynamic+i, opcodeModDynamicExplicit+i);

                extensions.registerInstruction (modCurrent + dynamics[i], "l",
                    opcodeModCurrentDynamic+i, opcodeModCurrentDynamicExplicit+i);

                extensions.registerFunction (get + dynamics[i] + getRatio, 'f', "",
                    opcodeGetDynamicGetRatio+i, opcodeGetDynamicGetRatioExplicit+i);
            }

            for (int i=0; i<numberOfSkills; ++i)
            {
                extensions.registerFunction (get + skills[i], 'l', "",
                    opcodeGetSkill+i, opcodeGetSkillExplicit+i);

                extensions.registerInstruction (set + skills[i], "l",
                    opcodeSetSkill+i, opcodeSetSkillExplicit+i);

                extensions.registerInstruction (mod + skills[i], "l",
                    opcodeModSkill+i, opcodeModSkillExplicit+i);
            }

            extensions.registerInstruction ("addspell", "c", opcodeAddSpell, opcodeAddSpellExplicit);
            extensions.registerInstruction ("removespell", "c", opcodeRemoveSpell,
                opcodeRemoveSpellExplicit);
            extensions.registerFunction ("getspell", 'l', "c", opcodeGetSpell, opcodeGetSpellExplicit);

            extensions.registerInstruction("pcraiserank","/S",opcodePCRaiseRank);
            extensions.registerInstruction("pclowerrank","/S",opcodePCLowerRank);
            extensions.registerInstruction("pcjoinfaction","/S",opcodePCJoinFaction);
            extensions.registerInstruction("moddisposition","l",opcodeModDisposition,
                opcodeModDispositionExplicit);
            extensions.registerFunction("getpcrank",'l',"/S",opcodeGetPCRank,opcodeGetPCRankExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            for (int i=0; i<numberOfAttributes; ++i)
            {
                interpreter.installSegment5 (opcodeGetAttribute+i, new OpGetAttribute<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeGetAttributeExplicit+i,
                    new OpGetAttribute<ExplicitRef> (i));

                interpreter.installSegment5 (opcodeSetAttribute+i, new OpSetAttribute<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeSetAttributeExplicit+i,
                    new OpSetAttribute<ExplicitRef> (i));

                interpreter.installSegment5 (opcodeModAttribute+i, new OpModAttribute<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeModAttributeExplicit+i,
                    new OpModAttribute<ExplicitRef> (i));
            }

            for (int i=0; i<numberOfDynamics; ++i)
            {
                interpreter.installSegment5 (opcodeGetDynamic+i, new OpGetDynamic<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeGetDynamicExplicit+i,
                    new OpGetDynamic<ExplicitRef> (i));

                interpreter.installSegment5 (opcodeSetDynamic+i, new OpSetDynamic<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeSetDynamicExplicit+i,
                    new OpSetDynamic<ExplicitRef> (i));

                interpreter.installSegment5 (opcodeModDynamic+i, new OpModDynamic<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeModDynamicExplicit+i,
                    new OpModDynamic<ExplicitRef> (i));

                interpreter.installSegment5 (opcodeModCurrentDynamic+i,
                    new OpModCurrentDynamic<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeModCurrentDynamicExplicit+i,
                    new OpModCurrentDynamic<ExplicitRef> (i));

                interpreter.installSegment5 (opcodeGetDynamicGetRatio+i,
                    new OpGetDynamicGetRatio<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeGetDynamicGetRatioExplicit+i,
                    new OpGetDynamicGetRatio<ExplicitRef> (i));
            }

            for (int i=0; i<numberOfSkills; ++i)
            {
                interpreter.installSegment5 (opcodeGetSkill+i, new OpGetSkill<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeGetSkillExplicit+i, new OpGetSkill<ExplicitRef> (i));

                interpreter.installSegment5 (opcodeSetSkill+i, new OpSetSkill<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeSetSkillExplicit+i, new OpSetSkill<ExplicitRef> (i));

                interpreter.installSegment5 (opcodeModSkill+i, new OpModSkill<ImplicitRef> (i));
                interpreter.installSegment5 (opcodeModSkillExplicit+i, new OpModSkill<ExplicitRef> (i));
            }

            interpreter.installSegment5 (opcodeAddSpell, new OpAddSpell<ImplicitRef>);
            interpreter.installSegment5 (opcodeAddSpellExplicit, new OpAddSpell<ExplicitRef>);
            interpreter.installSegment5 (opcodeRemoveSpell, new OpRemoveSpell<ImplicitRef>);
            interpreter.installSegment5 (opcodeRemoveSpellExplicit,
                new OpRemoveSpell<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetSpell, new OpGetSpell<ImplicitRef>);
            interpreter.installSegment5 (opcodeGetSpellExplicit, new OpGetSpell<ExplicitRef>);

            interpreter.installSegment3(opcodePCRaiseRank,new OpPCRaiseRank);
            interpreter.installSegment3(opcodePCLowerRank,new OpPCLowerRank);
            interpreter.installSegment3(opcodePCJoinFaction,new OpPCJoinFaction);
            interpreter.installSegment5(opcodeModDisposition,new OpModDisposition<ImplicitRef>);
            interpreter.installSegment5(opcodeModDispositionExplicit,new OpModDisposition<ExplicitRef>);
            interpreter.installSegment3(opcodeGetPCRank,new OpGetPCRank<ImplicitRef>);
            interpreter.installSegment3(opcodeGetPCRankExplicit,new OpGetPCRank<ExplicitRef>);
        }
    }
}
