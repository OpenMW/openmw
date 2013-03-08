
#include "guiextensions.hpp"

#include <boost/algorithm/string.hpp>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "interpretercontext.hpp"

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
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

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
                Misc::StringUtils::toLower(cell);
                runtime.pop();

                // "Will match complete or partial cells, so ShowMap, "Vivec" will show cells Vivec and Vivec, Fred's House as well."
                // http://www.uesp.net/wiki/Tes3Mod:ShowMap

                const MWWorld::Store<ESM::Cell> &cells =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Cell>();

                MWWorld::Store<ESM::Cell>::iterator it = cells.extBegin();
                for (; it != cells.extEnd(); ++it)
                {
                    std::string name = it->mName;
                    Misc::StringUtils::toLower(name);
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


        const int opcodeEnableBirthMenu = 0x200000e;
        const int opcodeEnableClassMenu = 0x200000f;
        const int opcodeEnableNameMenu = 0x2000010;
        const int opcodeEnableRaceMenu = 0x2000011;
        const int opcodeEnableStatsReviewMenu = 0x2000012;
        const int opcodeEnableInventoryMenu = 0x2000013;
        const int opcodeEnableMagicMenu = 0x2000014;
        const int opcodeEnableMapMenu = 0x2000015;
        const int opcodeEnableStatsMenu = 0x2000016;
        const int opcodeEnableRest = 0x2000017;
        const int opcodeShowRestMenu = 0x2000018;
        const int opcodeGetButtonPressed = 0x2000137;
        const int opcodeToggleFogOfWar = 0x2000145;
        const int opcodeToggleFullHelp = 0x2000151;
        const int opcodeShowMap = 0x20001a0;
        const int opcodeFillMap = 0x20001a1;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("enablebirthmenu", "", opcodeEnableBirthMenu);
            extensions.registerInstruction ("enableclassmenu", "", opcodeEnableClassMenu);
            extensions.registerInstruction ("enablenamemenu", "", opcodeEnableNameMenu);
            extensions.registerInstruction ("enableracemenu", "", opcodeEnableRaceMenu);
            extensions.registerInstruction ("enablestatreviewmenu", "",
opcodeEnableStatsReviewMenu);

            extensions.registerInstruction ("enableinventorymenu", "", opcodeEnableInventoryMenu);
            extensions.registerInstruction ("enablemagicmenu", "", opcodeEnableMagicMenu);
            extensions.registerInstruction ("enablemapmenu", "", opcodeEnableMapMenu);
            extensions.registerInstruction ("enablestatsmenu", "", opcodeEnableStatsMenu);

            extensions.registerInstruction ("enablerest", "", opcodeEnableRest);
            extensions.registerInstruction ("enablelevelupmenu", "", opcodeEnableRest);

            extensions.registerInstruction ("showrestmenu", "", opcodeShowRestMenu);

            extensions.registerFunction ("getbuttonpressed", 'l', "", opcodeGetButtonPressed);

            extensions.registerInstruction ("togglefogofwar", "", opcodeToggleFogOfWar);
            extensions.registerInstruction ("tfow", "", opcodeToggleFogOfWar);

            extensions.registerInstruction ("togglefullhelp", "", opcodeToggleFullHelp);
            extensions.registerInstruction ("tfh", "", opcodeToggleFullHelp);

            extensions.registerInstruction ("showmap", "S", opcodeShowMap);
            extensions.registerInstruction ("fillmap", "", opcodeFillMap);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeEnableBirthMenu,
                new OpShowDialogue (MWGui::GM_Birth));
            interpreter.installSegment5 (opcodeEnableClassMenu,
                new OpShowDialogue (MWGui::GM_Class));
            interpreter.installSegment5 (opcodeEnableNameMenu,
                new OpShowDialogue (MWGui::GM_Name));
            interpreter.installSegment5 (opcodeEnableRaceMenu,
                new OpShowDialogue (MWGui::GM_Race));
            interpreter.installSegment5 (opcodeEnableStatsReviewMenu,
                new OpShowDialogue (MWGui::GM_Review));

            interpreter.installSegment5 (opcodeEnableInventoryMenu,
                new OpEnableWindow (MWGui::GW_Inventory));
            interpreter.installSegment5 (opcodeEnableMagicMenu,
                new OpEnableWindow (MWGui::GW_Magic));
            interpreter.installSegment5 (opcodeEnableMapMenu,
                new OpEnableWindow (MWGui::GW_Map));
            interpreter.installSegment5 (opcodeEnableStatsMenu,
                new OpEnableWindow (MWGui::GW_Stats));

            interpreter.installSegment5 (opcodeEnableRest,
                new OpEnableRest ());

            interpreter.installSegment5 (opcodeShowRestMenu,
                new OpShowDialogue (MWGui::GM_RestBed));

            interpreter.installSegment5 (opcodeGetButtonPressed, new OpGetButtonPressed);

            interpreter.installSegment5 (opcodeToggleFogOfWar, new OpToggleFogOfWar);

            interpreter.installSegment5 (opcodeToggleFullHelp, new OpToggleFullHelp);

            interpreter.installSegment5 (opcodeShowMap, new OpShowMap);
            interpreter.installSegment5 (opcodeFillMap, new OpFillMap);
        }
    }
}
