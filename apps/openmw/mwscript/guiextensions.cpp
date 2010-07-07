
#include "guiextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "interpretercontext.hpp"

#include "../mwgui/guimanager.hpp"

namespace MWScript
{
    namespace Gui
    {    
        class OpEnableWindow : public Interpreter::Opcode0
        {
                MWGui::GuiManager::GuiWindow mWindow;
                
            public:
            
                OpEnableWindow (MWGui::GuiManager::GuiWindow window) : mWindow (window) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());
                    
                    context.getGuiManager().enableWindow (mWindow);
                } 
        };    
    
        class OpShowOneTimeDialogue : public Interpreter::Opcode0
        {
                MWGui::GuiManager::GuiOneTimeDialogue mOneTimeDialogue;
                
            public:
            
                OpShowOneTimeDialogue (MWGui::GuiManager::GuiOneTimeDialogue OneTimeDialogue)
                : mOneTimeDialogue (OneTimeDialogue)
                {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    context.getGuiManager().showOneTimeDialogue (mOneTimeDialogue);
                } 
        };    
            
        class OpShowDialogue : public Interpreter::Opcode0
        {
                MWGui::GuiManager::GuiDialogue mDialogue;
                
            public:
            
                OpShowDialogue (MWGui::GuiManager::GuiDialogue dialogue)
                : mDialogue (dialogue)
                {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());
                    
                    context.getGuiManager().enableDialogue (mDialogue);
                } 
        };  

        class OpEnableDialogue : public Interpreter::Opcode0
        {
                MWGui::GuiManager::GuiDialogue mDialogue;
                
            public:
            
                OpEnableDialogue (MWGui::GuiManager::GuiDialogue dialogue)
                : mDialogue (dialogue)
                {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());

                    context.getGuiManager().showDialogue (mDialogue);               
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
    
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("enablebirthmenu", "", opcodeEnableBirthMenu);
            extensions.registerInstruction ("enableclassmenu", "", opcodeEnableClassMenu);
            extensions.registerInstruction ("enablenamemenu", "", opcodeEnableNameMenu);
            extensions.registerInstruction ("enableracemenu", "", opcodeEnableRaceMenu);
            extensions.registerInstruction ("enablestatsreviewmenu", "",
                opcodeEnableStatsReviewMenu);

            extensions.registerInstruction ("enableinventorymenu", "", opcodeEnableInventoryMenu);
            extensions.registerInstruction ("enablemagicmenu", "", opcodeEnableMagicMenu);
            extensions.registerInstruction ("enablemapmenu", "", opcodeEnableMapMenu);
            extensions.registerInstruction ("enablestatsmenu", "", opcodeEnableStatsMenu);

            extensions.registerInstruction ("enablerestmenu", "", opcodeEnableRest);
            extensions.registerInstruction ("enablelevelupmenu", "", opcodeEnableRest);
            
            extensions.registerInstruction ("showrestmenu", "", opcodeShowRestMenu);
        }
        
        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeEnableBirthMenu,
                new OpShowOneTimeDialogue (MWGui::GuiManager::Gui_Birth));
            interpreter.installSegment5 (opcodeEnableClassMenu,
                new OpShowOneTimeDialogue (MWGui::GuiManager::Gui_Class));
            interpreter.installSegment5 (opcodeEnableNameMenu,
                new OpShowOneTimeDialogue (MWGui::GuiManager::Gui_Name));
            interpreter.installSegment5 (opcodeEnableRaceMenu,
                new OpShowOneTimeDialogue (MWGui::GuiManager::Gui_Race));
            interpreter.installSegment5 (opcodeEnableStatsReviewMenu,
                new OpShowOneTimeDialogue (MWGui::GuiManager::Gui_Review));

            interpreter.installSegment5 (opcodeEnableInventoryMenu,
                new OpEnableWindow (MWGui::GuiManager::Gui_Inventory));
            interpreter.installSegment5 (opcodeEnableMagicMenu,
                new OpEnableWindow (MWGui::GuiManager:: Gui_Magic));
            interpreter.installSegment5 (opcodeEnableMapMenu,
                new OpEnableWindow (MWGui::GuiManager::Gui_Map));
            interpreter.installSegment5 (opcodeEnableStatsMenu,
                new OpEnableWindow (MWGui::GuiManager::Gui_Status));

            interpreter.installSegment5 (opcodeEnableRest,
                new OpEnableDialogue (MWGui::GuiManager::Gui_Rest));

            interpreter.installSegment5 (opcodeShowRestMenu,
                new OpShowDialogue (MWGui::GuiManager::Gui_Rest));
        }
    }
}
