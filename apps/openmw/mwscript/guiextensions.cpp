#include "guiextensions.hpp"

#include <components/compiler/opcodes.hpp>

#include <components/interpreter/context.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/interpreter/runtime.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "ref.hpp"

namespace MWScript
{
    namespace Gui
    {
        class OpEnableWindow : public Interpreter::Opcode0
        {
            MWGui::GuiWindow mWindow;

        public:
            OpEnableWindow(MWGui::GuiWindow window)
                : mWindow(window)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getWindowManager()->allow(mWindow);
            }
        };

        class OpEnableRest : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getWindowManager()->enableRest();
            }
        };

        template <class R>
        class OpShowRestMenu : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                MWWorld::Ptr bed = R()(runtime, false);

                if (bed.isEmpty()
                    || !MWBase::Environment::get().getMechanicsManager()->sleepInBed(MWMechanics::getPlayer(), bed))
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Rest, bed);
            }
        };

        class OpShowDialogue : public Interpreter::Opcode0
        {
            MWGui::GuiMode mDialogue;

        public:
            OpShowDialogue(MWGui::GuiMode dialogue)
                : mDialogue(dialogue)
            {
            }

            void execute(Interpreter::Runtime& runtime) override
            {
                MWBase::Environment::get().getWindowManager()->pushGuiMode(mDialogue);
            }
        };

        class OpGetButtonPressed : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.push(MWBase::Environment::get().getWindowManager()->readPressedButton());
            }
        };

        class OpToggleFogOfWar : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.getContext().report(MWBase::Environment::get().getWindowManager()->toggleFogOfWar()
                        ? "Fog of war -> On"
                        : "Fog of war -> Off");
            }
        };

        class OpToggleFullHelp : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                runtime.getContext().report(MWBase::Environment::get().getWindowManager()->toggleFullHelp()
                        ? "Full help -> On"
                        : "Full help -> Off");
            }
        };

        class OpShowMap : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                std::string_view cell = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                // "Will match complete or partial cells, so ShowMap, "Vivec" will show cells Vivec and Vivec, Fred's
                // House as well." http://www.uesp.net/wiki/Tes3Mod:ShowMap

                const MWWorld::Store<ESM::Cell>& cells = MWBase::Environment::get().getESMStore()->get<ESM::Cell>();

                MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();

                for (auto it = cells.extBegin(); it != cells.extEnd(); ++it)
                {
                    const auto& cellName = it->mName;
                    if (Misc::StringUtils::ciStartsWith(cellName, cell))
                        winMgr->addVisitedLocation(cellName, it->getGridX(), it->getGridY());
                }
            }
        };

        class OpFillMap : public Interpreter::Opcode0
        {
        public:
            void execute(Interpreter::Runtime& runtime) override
            {
                const MWWorld::Store<ESM::Cell>& cells = MWBase::Environment::get().getESMStore()->get<ESM::Cell>();

                for (auto it = cells.extBegin(); it != cells.extEnd(); ++it)
                {
                    const std::string& name = it->mName;
                    if (!name.empty())
                        MWBase::Environment::get().getWindowManager()->addVisitedLocation(
                            name, it->getGridX(), it->getGridY());
                }
            }
        };

        class OpMenuTest : public Interpreter::Opcode1
        {
        public:
            void execute(Interpreter::Runtime& runtime, unsigned int arg0) override
            {
                int arg = 0;
                if (arg0 > 0)
                {
                    arg = runtime[0].mInteger;
                    runtime.pop();
                }

                if (arg == 0)
                {
                    MWGui::GuiMode modes[] = { MWGui::GM_Inventory, MWGui::GM_Container };

                    for (int i = 0; i < 2; ++i)
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
            void execute(Interpreter::Runtime& runtime) override
            {
                bool state = MWBase::Environment::get().getWindowManager()->setHudVisibility(
                    !MWBase::Environment::get().getWindowManager()->isHudVisible());
                runtime.getContext().report(state ? "GUI -> On" : "GUI -> Off");

                if (!state)
                {
                    while (MWBase::Environment::get().getWindowManager()->getMode()
                        != MWGui::GM_None) // don't use isGuiMode, or we get an infinite loop for modal message boxes!
                        MWBase::Environment::get().getWindowManager()->popGuiMode();
                }
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5<OpShowDialogue>(Compiler::Gui::opcodeEnableBirthMenu, MWGui::GM_Birth);
            interpreter.installSegment5<OpShowDialogue>(Compiler::Gui::opcodeEnableClassMenu, MWGui::GM_Class);
            interpreter.installSegment5<OpShowDialogue>(Compiler::Gui::opcodeEnableNameMenu, MWGui::GM_Name);
            interpreter.installSegment5<OpShowDialogue>(Compiler::Gui::opcodeEnableRaceMenu, MWGui::GM_Race);
            interpreter.installSegment5<OpShowDialogue>(Compiler::Gui::opcodeEnableStatsReviewMenu, MWGui::GM_Review);
            interpreter.installSegment5<OpShowDialogue>(Compiler::Gui::opcodeEnableLevelupMenu, MWGui::GM_Levelup);

            interpreter.installSegment5<OpEnableWindow>(Compiler::Gui::opcodeEnableInventoryMenu, MWGui::GW_Inventory);
            interpreter.installSegment5<OpEnableWindow>(Compiler::Gui::opcodeEnableMagicMenu, MWGui::GW_Magic);
            interpreter.installSegment5<OpEnableWindow>(Compiler::Gui::opcodeEnableMapMenu, MWGui::GW_Map);
            interpreter.installSegment5<OpEnableWindow>(Compiler::Gui::opcodeEnableStatsMenu, MWGui::GW_Stats);

            interpreter.installSegment5<OpEnableRest>(Compiler::Gui::opcodeEnableRest);

            interpreter.installSegment5<OpShowRestMenu<ImplicitRef>>(Compiler::Gui::opcodeShowRestMenu);
            interpreter.installSegment5<OpShowRestMenu<ExplicitRef>>(Compiler::Gui::opcodeShowRestMenuExplicit);

            interpreter.installSegment5<OpGetButtonPressed>(Compiler::Gui::opcodeGetButtonPressed);

            interpreter.installSegment5<OpToggleFogOfWar>(Compiler::Gui::opcodeToggleFogOfWar);

            interpreter.installSegment5<OpToggleFullHelp>(Compiler::Gui::opcodeToggleFullHelp);

            interpreter.installSegment5<OpShowMap>(Compiler::Gui::opcodeShowMap);
            interpreter.installSegment5<OpFillMap>(Compiler::Gui::opcodeFillMap);
            interpreter.installSegment3<OpMenuTest>(Compiler::Gui::opcodeMenuTest);
            interpreter.installSegment5<OpToggleMenus>(Compiler::Gui::opcodeToggleMenus);
        }
    }
}
