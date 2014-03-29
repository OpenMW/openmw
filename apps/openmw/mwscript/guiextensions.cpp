
#include "guiextensions.hpp"

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwbase/mechanicsmanager.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Gui
    {
        class OpEnableWindow : public Interpreter::Opcode0
        {
                MWGui::GuiWindow mWindow;

            public:

                OpEnableWindow (MWGui::GuiWindow window) : mWindow (window) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWindowManager()->allow (mWindow);
                }
        };

        class OpEnableRest : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWindowManager()->enableRest();
                }
        };

        template <class R>
        class OpShowRestMenu : public Interpreter::Opcode0
        {
        public:
            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWWorld::Ptr bed = R()(runtime, false);

                if (bed.isEmpty() || !MWBase::Environment::get().getMechanicsManager()->sleepInBed(MWBase::Environment::get().getWorld()->getPlayerPtr(),
                                                                             bed))
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_RestBed);
            }
        };

        class OpShowDialogue : public Interpreter::Opcode0
        {
                MWGui::GuiMode mDialogue;

            public:

                OpShowDialogue (MWGui::GuiMode dialogue)
                : mDialogue (dialogue)
                {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(mDialogue);
                }
        };

        class OpGetButtonPressed : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.push (MWBase::Environment::get().getWindowManager()->readPressedButton());
                }
        };

        class OpToggleFogOfWar : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWindowManager()->toggleFogOfWar();
                }
        };

        class OpToggleFullHelp : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWBase::Environment::get().getWindowManager()->toggleFullHelp();
                }
        };

        class OpShowMap : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                std::string cell = (runtime.getStringLiteral (runtime[0].mInteger));
                ::Misc::StringUtils::toLower(cell);
                runtime.pop();

                // "Will match complete or partial cells, so ShowMap, "Vivec" will show cells Vivec and Vivec, Fred's House as well."
                // http://www.uesp.net/wiki/Tes3Mod:ShowMap

                const MWWorld::Store<ESM::Cell> &cells =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Cell>();

                MWWorld::Store<ESM::Cell>::iterator it = cells.extBegin();
                for (; it != cells.extEnd(); ++it)
                {
                    std::string name = it->mName;
                    ::Misc::StringUtils::toLower(name);
                    if (name.find(cell) != std::string::npos)
                        MWBase::Environment::get().getWindowManager()->addVisitedLocation (
                            it->mName,
                            it->getGridX(),
                            it->getGridY()
                        );
                }
            }
        };

        class OpFillMap : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                const MWWorld::Store<ESM::Cell> &cells =
                    MWBase::Environment::get().getWorld ()->getStore().get<ESM::Cell>();

                MWWorld::Store<ESM::Cell>::iterator it = cells.extBegin();
                for (; it != cells.extEnd(); ++it)
                {
                    std::string name = it->mName;
                    if (name != "")
                        MWBase::Environment::get().getWindowManager()->addVisitedLocation (
                            name,
                            it->getGridX(),
                            it->getGridY()
                        );
                }
            }
        };


        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableBirthMenu,
                new OpShowDialogue (MWGui::GM_Birth));
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableClassMenu,
                new OpShowDialogue (MWGui::GM_Class));
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableNameMenu,
                new OpShowDialogue (MWGui::GM_Name));
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableRaceMenu,
                new OpShowDialogue (MWGui::GM_Race));
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableStatsReviewMenu,
                new OpShowDialogue (MWGui::GM_Review));

            interpreter.installSegment5 (Compiler::Gui::opcodeEnableInventoryMenu,
                new OpEnableWindow (MWGui::GW_Inventory));
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableMagicMenu,
                new OpEnableWindow (MWGui::GW_Magic));
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableMapMenu,
                new OpEnableWindow (MWGui::GW_Map));
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableStatsMenu,
                new OpEnableWindow (MWGui::GW_Stats));

            interpreter.installSegment5 (Compiler::Gui::opcodeEnableRest,
                new OpEnableRest ());

            interpreter.installSegment5 (Compiler::Gui::opcodeShowRestMenu,
                new OpShowRestMenu<ImplicitRef>);
            interpreter.installSegment5 (Compiler::Gui::opcodeShowRestMenuExplicit, new OpShowRestMenu<ExplicitRef>);

            interpreter.installSegment5 (Compiler::Gui::opcodeGetButtonPressed, new OpGetButtonPressed);

            interpreter.installSegment5 (Compiler::Gui::opcodeToggleFogOfWar, new OpToggleFogOfWar);

            interpreter.installSegment5 (Compiler::Gui::opcodeToggleFullHelp, new OpToggleFullHelp);

            interpreter.installSegment5 (Compiler::Gui::opcodeShowMap, new OpShowMap);
            interpreter.installSegment5 (Compiler::Gui::opcodeFillMap, new OpFillMap);
        }
    }
}
