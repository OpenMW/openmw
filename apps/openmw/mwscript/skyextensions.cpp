
#include "skyextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Sky
    {
        class OpToggleSky : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());
                    
                    context.getWorld().toggleSky();
                } 
        };      
        
        class OpTurnMoonWhite : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());
                    
                    context.getWorld().setMoonColour (false);
                } 
        };          

        class OpTurnMoonRed : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());
                    
                    context.getWorld().setMoonColour (true);
                } 
        };     
        
        class OpGetMasserPhase : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());
         
                    runtime.push (context.getWorld().getMasserPhase());
                } 
        };               

        class OpGetSecundaPhase : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    InterpreterContext& context =
                        static_cast<InterpreterContext&> (runtime.getContext());
         
                    runtime.push (context.getWorld().getSecundaPhase());
                } 
        };               
    
        const int opcodeToggleSky = 0x2000021;
        const int opcodeTurnMoonWhite = 0x2000022;
        const int opcodeTurnMoonRed = 0x2000023;
        const int opcodeGetMasserPhase = 0x2000024;
        const int opcodeGetSecundaPhase = 0x2000025;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("togglesky", "", opcodeToggleSky);
            extensions.registerInstruction ("ts", "", opcodeToggleSky);
            extensions.registerInstruction ("turnmoonwhite", "", opcodeTurnMoonWhite);
            extensions.registerInstruction ("turnmoonred", "", opcodeTurnMoonRed);
            extensions.registerFunction ("getmasserphase", 'l', "", opcodeGetMasserPhase);
            extensions.registerFunction ("getsecundaphase", 'l', "", opcodeGetSecundaPhase);
        }
            
        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeToggleSky, new OpToggleSky);
            interpreter.installSegment5 (opcodeTurnMoonWhite, new OpTurnMoonWhite);
            interpreter.installSegment5 (opcodeTurnMoonRed, new OpTurnMoonRed);
            interpreter.installSegment5 (opcodeGetMasserPhase, new OpGetMasserPhase);
            interpreter.installSegment5 (opcodeGetSecundaPhase, new OpGetSecundaPhase);
        }    
    }
}

