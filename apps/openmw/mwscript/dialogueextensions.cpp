#include "dialogueextensions.hpp"

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>
#include <components/debug/debuglog.hpp>
#include <components/interpreter/context.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "ref.hpp"

namespace MWScript
{
    namespace Dialogue
    {
        template <class R>
        class OpJournal : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime, false); // required=false
                if (ptr.isEmpty())
                    ptr = MWBase::Environment::get().getWorld()->getPlayerPtr();

                ESM::RefId quest = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Integer index = runtime[0].mInteger;
                runtime.pop();

                // Invoking Journal with a non-existing index is allowed, and triggers no errors. Seriously? :(
                try
                {
                    MWBase::Environment::get().getJournal()->addEntry(quest, index, ptr);
                }
                catch (...)
                {
                    if (MWBase::Environment::get().getJournal()->getJournalIndex(quest) < index)
                        MWBase::Environment::get().getJournal()->setJournalIndex(quest, index);
                }
            }
        };

        class OpSetJournalIndex : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId quest = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                Interpreter::Type_Integer index = runtime[0].mInteger;
                runtime.pop();

                MWBase::Environment::get().getJournal()->setJournalIndex(quest, index);
            }
        };

        class OpGetJournalIndex : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId quest = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                int index = MWBase::Environment::get().getJournal()->getJournalIndex(quest);

                runtime.push(index);
            }
        };

        class OpFillJournal : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const MWWorld::Store<ESM::Dialogue>& dialogues
                    = MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>();
                MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();
                MWBase::Journal* journal = MWBase::Environment::get().getJournal();
                MWBase::DialogueManager* dialogueManager = MWBase::Environment::get().getDialogueManager();

                for (const auto& dialogue : dialogues)
                {
                    if (dialogue.mType == ESM::Dialogue::Type::Journal)
                    {
                        for (const auto& journalInfo : dialogue.mInfoOrder.getOrderedInfo())
                        {
                            if (journalInfo.mQuestStatus != ESM::DialInfo::QS_Name)
                                journal->addEntry(dialogue.mId, journalInfo.mData.mJournalIndex, playerPtr);
                        }
                    }
                    else if (dialogue.mType == ESM::Dialogue::Type::Topic)
                    {
                        for (const auto& topicInfo : dialogue.mInfoOrder.getOrderedInfo())
                        {
                            journal->addTopic(dialogue.mId, topicInfo.mId, playerPtr);
                        }
                        dialogueManager->addTopic(dialogue.mId);
                    }
                }
            }
        };

        class OpAddTopic : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId topic = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                if (!MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>().search(topic))
                {
                    runtime.getContext().report(
                        "Failed to add topic '" + topic.getRefIdString() + "': topic record not found");
                    return;
                }

                MWBase::Environment::get().getDialogueManager()->addTopic(topic);
            }
        };

        class OpChoice : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                MWBase::DialogueManager* dialogue = MWBase::Environment::get().getDialogueManager();
                while (arg0 > 0)
                {
                    std::string_view question = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();
                    arg0 = arg0 - 1;
                    Interpreter::Type_Integer choice = 1;
                    if (arg0 > 0)
                    {
                        choice = runtime[0].mInteger;
                        runtime.pop();
                        arg0 = arg0 - 1;
                    }
                    dialogue->addChoice(question, choice);
                }
            }
        };

        template <class R>
        class OpForceGreeting : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                if (!ptr.getRefData().isEnabled())
                    return;

                if (!ptr.getClass().isActor())
                {
                    const std::string error = "Warning: \"forcegreeting\" command works only for actors.";
                    runtime.getContext().report(error);
                    Log(Debug::Warning) << error;
                    return;
                }

                bool greetWerewolves = false;
                const ESM::RefId& script = ptr.getClass().getScript(ptr);
                if (!script.empty())
                    greetWerewolves = ptr.getRefData().getLocals().hasVar(script, "allowwerewolfforcegreeting");

                const MWWorld::Ptr& player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                if (player.getClass().getNpcStats(player).isWerewolf() && !greetWerewolves)
                    return;

                MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Dialogue, ptr);
            }
        };

        class OpGoodbye : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getDialogueManager()->goodbye();
            }
        };

        template <class R>
        class OpModReputation : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();

                ptr.getClass().getNpcStats(ptr).setReputation(ptr.getClass().getNpcStats(ptr).getReputation() + value);
            }
        };

        template <class R>
        class OpSetReputation : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);
                Interpreter::Type_Integer value = runtime[0].mInteger;
                runtime.pop();

                ptr.getClass().getNpcStats(ptr).setReputation(value);
            }
        };

        template <class R>
        class OpGetReputation : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                runtime.push(ptr.getClass().getNpcStats(ptr).getReputation());
            }
        };

        template <class R>
        class OpSameFaction : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

                runtime.push(player.getClass().getNpcStats(player).isInFaction(ptr.getClass().getPrimaryFaction(ptr)));
            }
        };

        class OpModFactionReaction : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId faction1 = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                ESM::RefId faction2 = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                int modReaction = runtime[0].mInteger;
                runtime.pop();

                MWBase::Environment::get().getDialogueManager()->modFactionReaction(faction1, faction2, modReaction);
            }
        };

        class OpGetFactionReaction : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId faction1 = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                ESM::RefId faction2 = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                runtime.push(MWBase::Environment::get().getDialogueManager()->getFactionReaction(faction1, faction2));
            }
        };

        class OpSetFactionReaction : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                ESM::RefId faction1 = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                ESM::RefId faction2 = ESM::RefId::stringRefId(runtime.getStringLiteral(runtime[0].mInteger));
                runtime.pop();

                int newValue = runtime[0].mInteger;
                runtime.pop();

                MWBase::Environment::get().getDialogueManager()->setFactionReaction(faction1, faction2, newValue);
            }
        };

        template <class R>
        class OpClearInfoActor : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr ptr = R()(runtime);

                MWBase::Environment::get().getDialogueManager()->clearInfoActor(ptr);
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5<OpJournal<ImplicitRef>>(Compiler::Dialogue::opcodeJournal);
            interpreter.installSegment5<OpJournal<ExplicitRef>>(Compiler::Dialogue::opcodeJournalExplicit);
            interpreter.installSegment5<OpSetJournalIndex>(Compiler::Dialogue::opcodeSetJournalIndex);
            interpreter.installSegment5<OpGetJournalIndex>(Compiler::Dialogue::opcodeGetJournalIndex);
            interpreter.installSegment5<OpFillJournal>(Compiler::Dialogue::opcodeFillJournal);
            interpreter.installSegment5<OpAddTopic>(Compiler::Dialogue::opcodeAddTopic);
            interpreter.installSegment3<OpChoice>(Compiler::Dialogue::opcodeChoice);
            interpreter.installSegment5<OpForceGreeting<ImplicitRef>>(Compiler::Dialogue::opcodeForceGreeting);
            interpreter.installSegment5<OpForceGreeting<ExplicitRef>>(Compiler::Dialogue::opcodeForceGreetingExplicit);
            interpreter.installSegment5<OpGoodbye>(Compiler::Dialogue::opcodeGoodbye);
            interpreter.installSegment5<OpGetReputation<ImplicitRef>>(Compiler::Dialogue::opcodeGetReputation);
            interpreter.installSegment5<OpSetReputation<ImplicitRef>>(Compiler::Dialogue::opcodeSetReputation);
            interpreter.installSegment5<OpModReputation<ImplicitRef>>(Compiler::Dialogue::opcodeModReputation);
            interpreter.installSegment5<OpSetReputation<ExplicitRef>>(Compiler::Dialogue::opcodeSetReputationExplicit);
            interpreter.installSegment5<OpModReputation<ExplicitRef>>(Compiler::Dialogue::opcodeModReputationExplicit);
            interpreter.installSegment5<OpGetReputation<ExplicitRef>>(Compiler::Dialogue::opcodeGetReputationExplicit);
            interpreter.installSegment5<OpSameFaction<ImplicitRef>>(Compiler::Dialogue::opcodeSameFaction);
            interpreter.installSegment5<OpSameFaction<ExplicitRef>>(Compiler::Dialogue::opcodeSameFactionExplicit);
            interpreter.installSegment5<OpModFactionReaction>(Compiler::Dialogue::opcodeModFactionReaction);
            interpreter.installSegment5<OpSetFactionReaction>(Compiler::Dialogue::opcodeSetFactionReaction);
            interpreter.installSegment5<OpGetFactionReaction>(Compiler::Dialogue::opcodeGetFactionReaction);
            interpreter.installSegment5<OpClearInfoActor<ImplicitRef>>(Compiler::Dialogue::opcodeClearInfoActor);
            interpreter.installSegment5<OpClearInfoActor<ExplicitRef>>(
                Compiler::Dialogue::opcodeClearInfoActorExplicit);
        }
    }

}
