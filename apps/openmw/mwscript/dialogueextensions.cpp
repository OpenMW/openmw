
#include "dialogueextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/journal.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Dialogue
    {
        class OpJournal : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string quest = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer index = runtime[0].mInteger;
                    runtime.pop();

                    MWBase::Environment::get().getJournal()->addEntry (quest, index);
                }
        };

        class OpSetJournalIndex : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string quest = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer index = runtime[0].mInteger;
                    runtime.pop();

                    MWBase::Environment::get().getJournal()->setJournalIndex (quest, index);
                }
        };

        class OpGetJournalIndex : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string quest = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    int index = MWBase::Environment::get().getJournal()->getJournalIndex (quest);

                    runtime.push (index);

                }
        };

        class OpAddTopic : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    std::string topic = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWBase::Environment::get().getDialogueManager()->addTopic(topic);
                }
        };

        class OpChoice : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {
                    MWBase::DialogueManager* dialogue = MWBase::Environment::get().getDialogueManager();
                    while(arg0>0)
                    {
                        std::string question = runtime.getStringLiteral (runtime[0].mInteger);
                        runtime.pop();
                        arg0 = arg0 -1;
                        Interpreter::Type_Integer choice = 1;
                        if(arg0>0)
                        {
                            choice = runtime[0].mInteger;
                            runtime.pop();
                            arg0 = arg0 -1;
                        }
                        dialogue->askQuestion(question,choice);
                    }
                }
        };

        template<class R>
        class OpForceGreeting : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWBase::Environment::get().getDialogueManager()->startDialogue (ptr);
                }
        };

        class OpGoodbye : public Interpreter::Opcode0
        {
            public:

                virtual void execute(Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getDialogueManager()->goodbye();
                }
        };

        template<class R>
        class OpModReputation : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get(ptr).getNpcStats (ptr).setReputation (MWWorld::Class::get(ptr).getNpcStats (ptr).getReputation () + value);
                }
        };

        template<class R>
        class OpSetReputation : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);
                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Class::get(ptr).getNpcStats (ptr).setReputation (value);
                }
        };

        template<class R>
        class OpGetReputation : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    runtime.push (MWWorld::Class::get(ptr).getNpcStats (ptr).getReputation ());
                }
        };

        template<class R>
        class OpSameFaction : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer();

                    runtime.push (MWWorld::Class::get(ptr).getNpcStats (ptr).isSameFaction (MWWorld::Class::get(player).getNpcStats (player)));
                }
        };

        const int opcodeJournal = 0x2000133;
        const int opcodeSetJournalIndex = 0x2000134;
        const int opcodeGetJournalIndex = 0x2000135;
        const int opcodeAddTopic = 0x200013a;
        const int opcodeChoice = 0x2000a;
        const int opcodeForceGreeting = 0x200014f;
        const int opcodeForceGreetingExplicit = 0x2000150;
        const int opcodeGoodbye = 0x2000152;
        const int opcodeSetReputation = 0x20001ad;
        const int opcodeModReputation = 0x20001ae;
        const int opcodeSetReputationExplicit = 0x20001af;
        const int opcodeModReputationExplicit = 0x20001b0;
        const int opcodeGetReputation = 0x20001b1;
        const int opcodeGetReputationExplicit = 0x20001b2;
        const int opcodeSameFaction = 0x20001b5;
        const int opcodeSameFactionExplicit = 0x20001b6;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("journal", "cl", opcodeJournal);
            extensions.registerInstruction ("setjournalindex", "cl", opcodeSetJournalIndex);
            extensions.registerFunction ("getjournalindex", 'l', "c", opcodeGetJournalIndex);
            extensions.registerInstruction ("addtopic", "S" , opcodeAddTopic);
            extensions.registerInstruction ("choice", "/SlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSl", opcodeChoice);
            extensions.registerInstruction("forcegreeting","",opcodeForceGreeting);
            extensions.registerInstruction("forcegreeting","",opcodeForceGreeting,
                opcodeForceGreetingExplicit);
            extensions.registerInstruction("goodbye", "", opcodeGoodbye);
            extensions.registerInstruction("setreputation", "l", opcodeSetReputation,
                opcodeSetReputationExplicit);
            extensions.registerInstruction("modreputation", "l", opcodeModReputation,
                opcodeModReputationExplicit);
            extensions.registerFunction("getreputation", 'l', "", opcodeGetReputation,
                opcodeGetReputationExplicit);
            extensions.registerFunction("samefaction", 'l', "", opcodeSameFaction,
                opcodeSameFactionExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeJournal, new OpJournal);
            interpreter.installSegment5 (opcodeSetJournalIndex, new OpSetJournalIndex);
            interpreter.installSegment5 (opcodeGetJournalIndex, new OpGetJournalIndex);
            interpreter.installSegment5 (opcodeAddTopic, new OpAddTopic);
            interpreter.installSegment3 (opcodeChoice,new OpChoice);
            interpreter.installSegment5 (opcodeForceGreeting, new OpForceGreeting<ImplicitRef>);
            interpreter.installSegment5 (opcodeForceGreetingExplicit, new OpForceGreeting<ExplicitRef>);
            interpreter.installSegment5 (opcodeGoodbye, new OpGoodbye);
            interpreter.installSegment5 (opcodeGetReputation, new OpGetReputation<ImplicitRef>);
            interpreter.installSegment5 (opcodeSetReputation, new OpSetReputation<ImplicitRef>);
            interpreter.installSegment5 (opcodeModReputation, new OpModReputation<ImplicitRef>);
            interpreter.installSegment5 (opcodeSetReputationExplicit, new OpSetReputation<ExplicitRef>);
            interpreter.installSegment5 (opcodeModReputationExplicit, new OpModReputation<ExplicitRef>);
            interpreter.installSegment5 (opcodeGetReputationExplicit, new OpGetReputation<ExplicitRef>);
            interpreter.installSegment5 (opcodeSameFaction, new OpSameFaction<ImplicitRef>);
            interpreter.installSegment5 (opcodeSameFactionExplicit, new OpSameFaction<ExplicitRef>);
        }
    }

}
