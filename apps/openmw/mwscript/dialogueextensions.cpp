
#include "dialogueextensions.hpp"

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/journal.hpp"

#include "../mwworld/class.hpp"
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

                    MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();

                    runtime.push (MWWorld::Class::get(ptr).getNpcStats (ptr).isSameFaction (MWWorld::Class::get(player).getNpcStats (player)));
                }
        };


        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::Dialogue::opcodeJournal, new OpJournal);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeSetJournalIndex, new OpSetJournalIndex);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeGetJournalIndex, new OpGetJournalIndex);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeAddTopic, new OpAddTopic);
            interpreter.installSegment3 (Compiler::Dialogue::opcodeChoice,new OpChoice);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeForceGreeting, new OpForceGreeting<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeForceGreetingExplicit, new OpForceGreeting<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeGoodbye, new OpGoodbye);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeGetReputation, new OpGetReputation<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeSetReputation, new OpSetReputation<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeModReputation, new OpModReputation<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeSetReputationExplicit, new OpSetReputation<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeModReputationExplicit, new OpModReputation<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeGetReputationExplicit, new OpGetReputation<ExplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeSameFaction, new OpSameFaction<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Dialogue::opcodeSameFactionExplicit, new OpSameFaction<ExplicitRef>);
        }
    }

}
