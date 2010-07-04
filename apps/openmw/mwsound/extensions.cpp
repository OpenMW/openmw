
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
        class OpSay : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string file = runtime.getStringLiteral (runtime[0]);
                    runtime.pop();

                    std::string text = runtime.getStringLiteral (runtime[0]);
                    runtime.pop();
                        
                    context.getSoundManager().say (context.getReference(), file, text,
                        context);
                } 
        };   
            
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
    
        class OpStreamMusic : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0]);
                    runtime.pop();
                        
                    context.getSoundManager().streamMusic (sound, context);
                } 
        };      

        class OpPlaySound : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0]);
                    runtime.pop();
                        
                    context.getSoundManager().playSound (sound, 1.0, 1.0, context);
                } 
        };      
    
        class OpPlaySoundVP : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0]);
                    runtime.pop();
                    
                    float volume = *reinterpret_cast<float *> (&runtime[0]);
                    runtime.pop();

                    float pitch = *reinterpret_cast<float *> (&runtime[0]);
                    runtime.pop();
                        
                    context.getSoundManager().playSound (sound, volume, pitch, context);
                } 
        };      
    
        class OpPlaySound3D : public Interpreter::Opcode0
        {
                bool mLoop;
                
            public:
            
                OpPlaySound3D (bool loop) : mLoop (loop) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0]);
                    runtime.pop();
                                            
                    context.getSoundManager().playSound3D (context.getReference(), sound,
                        1.0, 1.0, mLoop, context);
                } 
        };      
    
        class OpPlaySoundVP3D : public Interpreter::Opcode0
        {
                bool mLoop;
                     
            public:
            
                OpPlaySoundVP3D (bool loop) : mLoop (loop) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0]);
                    runtime.pop();
                                        
                    float volume = *reinterpret_cast<float *> (&runtime[0]);
                    runtime.pop();

                    float pitch = *reinterpret_cast<float *> (&runtime[0]);
                    runtime.pop();
                        
                    context.getSoundManager().playSound3D (context.getReference(), sound, volume,
                        pitch, mLoop, context);

                } 
        };     

        class OpStopSound : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0]);
                    runtime.pop();
                                            
                    context.getSoundManager().stopSound3D (context.getReference(), sound, context);
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
        const int opcodePlayLoopSound3D = 0x2000008;
        const int opcodePlayLoopSound3DVP = 0x2000009;
        const int opcodeStopSound = 0x200000a;
        const int opcodeGetSoundPlaying = 0x200000b;
    }

    void registerExtensions (Compiler::Extensions& extensions)
    {
        extensions.registerInstruction ("say", "SS", Script::opcodeSay);
        extensions.registerFunction ("saydone", 'l', "", Script::opcodeSayDone);
        extensions.registerInstruction ("streammusic", "S", Script::opcodeStreamMusic);
        extensions.registerInstruction ("playsound", "c", Script::opcodePlaySound);
        extensions.registerInstruction ("playsoundvp", "cff", Script::opcodePlaySoundVP);
        extensions.registerInstruction ("playsound3d", "c", Script::opcodePlaySound3D);
        extensions.registerInstruction ("playsound3dvp", "cff", Script::opcodePlaySound3DVP);
        extensions.registerInstruction ("playloopsound3d", "c", Script::opcodePlayLoopSound3D);
        extensions.registerInstruction ("playloopsound3dvp", "cff",
            Script::opcodePlayLoopSound3DVP);
        extensions.registerInstruction ("stopsound", "c", Script::opcodeStopSound);
        extensions.registerFunction ("getsoundplaying", 'l', "c", Script::opcodeGetSoundPlaying);   
    }
    
    void installOpcodes (Interpreter::Interpreter& interpreter)
    {
        interpreter.installSegment5 (Script::opcodeSay, new Script::OpSay);
        interpreter.installSegment5 (Script::opcodeSayDone, new Script::OpSayDone);
        interpreter.installSegment5 (Script::opcodeStreamMusic, new Script::OpStreamMusic);
        interpreter.installSegment5 (Script::opcodePlaySound, new Script::OpPlaySound);
        interpreter.installSegment5 (Script::opcodePlaySoundVP, new Script::OpPlaySoundVP);
        interpreter.installSegment5 (Script::opcodePlaySound3D, new Script::OpPlaySound3D (false));
        interpreter.installSegment5 (Script::opcodePlaySound3DVP,
            new Script::OpPlaySoundVP3D (false));
        interpreter.installSegment5 (Script::opcodePlayLoopSound3D,
            new Script::OpPlaySound3D (true));
        interpreter.installSegment5 (Script::opcodePlayLoopSound3DVP,
            new Script::OpPlaySoundVP3D (true));
        interpreter.installSegment5 (Script::opcodeStopSound, new Script::OpStopSound);
        interpreter.installSegment5 (Script::opcodeGetSoundPlaying, new Script::OpGetSoundPlaying);
    }
}
