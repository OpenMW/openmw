#include "guiextensions.hpp"

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

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

                if (bed.isEmpty() || !MWBase::Environment::get().getMechanicsManager()->sleepInBed(MWMechanics::getPlayer(),
                                                                             bed))
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Rest, bed);
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
                    runtime.getContext().report(MWBase::Environment::get().getWindowManager()->toggleFogOfWar() ? "Fog of war -> On"
                                                                                                                : "Fog of war -> Off");
                }
        };

        class OpToggleFullHelp : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    runtime.getContext().report(MWBase::Environment::get().getWindowManager()->toggleFullHelp() ? "Full help -> On"
                                                                                                                : "Full help -> Off");
                }
        };

        class OpShowMap : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                std::string cell = (runtime.getStringLiteral (runtime[0].mInteger));
                ::Misc::StringUtils::lowerCaseInPlace(cell);
                runtime.pop();

                // "Will match complete or partial cells, so ShowMap, "Vivec" will show cells Vivec and Vivec, Fred's House as well."
                // http://www.uesp.net/wiki/Tes3Mod:ShowMap

                const MWWorld::Store<ESM::Cell> &cells =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Cell>();

                MWWorld::Store<ESM::Cell>::iterator it = cells.extBegin();
                for (; it != cells.extEnd(); ++it)
                {
                    std::string name = it->mName;
                    ::Misc::StringUtils::lowerCaseInPlace(name);
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

        class OpMenuTest : public Interpreter::Opcode1
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
            {
                int arg=0;
                if(arg0>0)
                {
                    arg = runtime[0].mInteger;
                    runtime.pop();
                }


                if (arg == 0)
                {
                    MWGui::GuiMode modes[] = { MWGui::GM_Inventory, MWGui::GM_Container };

                    for (int i=0; i<2; ++i)
                    {
                        if (MWBase::Environment::get().getWindowManager()->containsMode(modes[i]))
                            MWBase::Environment::get().getWindowManager()->removeGuiMode(modes[i]);
                    }
                }
                else
                {
                    MWGui::GuiWindow gw = MWGui::GW_None;
                    if (arg == 3)
                        gw = MWGui::GW_Stats;
                    if (arg == 4)
                        gw = MWGui::GW_Inventory;
                    if (arg == 5)
                        gw = MWGui::GW_Magic;
                    if (arg == 6)
                        gw = MWGui::GW_Map;

                    MWBase::Environment::get().getWindowManager()->pinWindow(gw);
                }
            }
        };

        class OpToggleMenus : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                bool state = MWBase::Environment::get().getWindowManager()->toggleHud();
                runtime.getContext().report(state ? "GUI -> On" : "GUI -> Off");

                if (!state)
                {
                    while (MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_None) // don't use isGuiMode, or we get an infinite loop for modal message boxes!
                        MWBase::Environment::get().getWindowManager()->popGuiMode();
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
            interpreter.installSegment5 (Compiler::Gui::opcodeEnableLevelupMenu,
                new OpShowDialogue (MWGui::GM_Levelup));

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
            interpreter.installSegment3 (Compiler::Gui::opcodeMenuTest, new OpMenuTest);
            interpreter.installSegment5 (Compiler::Gui::opcodeToggleMenus, new OpToggleMenus);
        }
    }
}
