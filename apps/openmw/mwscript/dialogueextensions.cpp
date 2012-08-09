
#include "dialogueextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/journal.hpp"

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

        const int opcodeJournal = 0x2000133;
        const int opcodeSetJournalIndex = 0x2000134;
        const int opcodeGetJournalIndex = 0x2000135;
        const int opcodeAddTopic = 0x200013a;
        const int opcodeChoice = 0x2000a;
        const int opcodeForceGreeting = 0x200014f;
        const int opcodeForceGreetingExplicit = 0x2000150;
        const int opcodeGoodbye = 0x2000152;

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
        }
    }

}
