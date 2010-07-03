
#include "extensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwscript/interpretercontext.hpp"

#include "../mwworld/world.hpp"

#include "soundmanager.hpp"

namespace MWSound
{
    namespace Script
    {
        class OpSayDone : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                        
                    runtime.push (context.getSoundManager().sayDone (context.getReference(),
                        context));
                } 
        };    
    
        class OpGetSoundPlaying : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                        
                    int index = runtime[0];
                    runtime.pop();
                        
                    runtime.push (context.getSoundManager().getSoundPlaying (
                        context.getReference(), runtime.getStringLiteral (index), context));
                } 
        };
            
        const int opcodeSay = 0x2000001;
        const int opcodeSayDone = 0x2000002;
        const int opcodeStreamMusic = 0x2000003;
        const int opcodePlaySound = 0x2000004;
        const int opcodePlaySoundVP = 0x2000005;
        const int opcodePlaySound3D = 0x2000006;
        const int opcodePlaySound3DVP = 0x2000007;
        const int opcodeStopSound = 0x2000008;
        const int opcodeGetSoundPlaying = 0x2000009;
    }

    // TODO opcodeSay, opcodeStreamMusic, opcodePlaySound, opcodePlaySoundVP,
    // opcodePlaySound, opcodePlaySound, opcodeStopSound

    void registerExtensions (Compiler::Extensions& extensions)
    {
        extensions.registerFunction ("saydone", 'l', "", Script::opcodeSayDone);

        extensions.registerFunction ("getsoundplaying", 'l', "S", Script::opcodeGetSoundPlaying);
        
    }
    
    void installOpcodes (Interpreter::Interpreter& interpreter)
    {
        interpreter.installSegment5 (Script::opcodeSayDone, new Script::OpSayDone);

        interpreter.installSegment5 (Script::opcodeGetSoundPlaying, new Script::OpGetSoundPlaying);
    }
}
