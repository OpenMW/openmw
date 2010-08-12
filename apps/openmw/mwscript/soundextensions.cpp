
#include "extensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "interpretercontext.hpp"

#include "../mwworld/world.hpp"

#include "../mwsound/soundmanager.hpp"

namespace MWScript
{
    namespace Sound
    {
        class OpSay : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string file = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string text = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    context.getSoundManager().say (context.getReference(), file, text);
                    context.messageBox (text);
                } 
        };   
            
        class OpSayDone : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                        
                    runtime.push (context.getSoundManager().sayDone (context.getReference()));
                } 
        };    
    
        class OpStreamMusic : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                        
                    context.getSoundManager().streamMusic (sound);
                } 
        };      

        class OpPlaySound : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                        
                    context.getSoundManager().playSound (sound, 1.0, 1.0);
                } 
        };      
    
        class OpPlaySoundVP : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    
                    Interpreter::Type_Float volume = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float pitch = runtime[0].mFloat;
                    runtime.pop();
                        
                    context.getSoundManager().playSound (sound, volume, pitch);
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
                   
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                            
                    context.getSoundManager().playSound3D (context.getReference(), sound,
                        1.0, 1.0, mLoop);
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
                   
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                        
                    Interpreter::Type_Float volume = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float pitch = runtime[0].mFloat;
                    runtime.pop();
                        
                    context.getSoundManager().playSound3D (context.getReference(), sound, volume,
                        pitch, mLoop);

                } 
        };     

        class OpStopSound : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                            
                    context.getSoundManager().stopSound3D (context.getReference(), sound);
                } 
        };      
                                
        class OpGetSoundPlaying : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                        
                    int index = runtime[0].mInteger;
                    runtime.pop();
                        
                    runtime.push (context.getSoundManager().getSoundPlaying (
                        context.getReference(), runtime.getStringLiteral (index)));
                } 
        };
        
        class OpSayExplicit : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                   
                    std::string file = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string text = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                        
                    context.getSoundManager().say (context.getWorld().getPtr (id, true),
                        file, text);
                } 
        };   
            
        class OpSayDoneExplicit : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                            
                    runtime.push (context.getSoundManager().sayDone (
                        context.getWorld().getPtr (id, true)));
                } 
        };    
        
        class OpPlaySound3DExplicit : public Interpreter::Opcode0
        {
                bool mLoop;
                
            public:
            
                OpPlaySound3DExplicit (bool loop) : mLoop (loop) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                       
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                            
                    context.getSoundManager().playSound3D (
                        context.getWorld().getPtr (id, true), sound, 1.0, 1.0, mLoop);
                } 
        };      
    
        class OpPlaySoundVP3DExplicit : public Interpreter::Opcode0
        {
                bool mLoop;
                     
            public:
            
                OpPlaySoundVP3DExplicit (bool loop) : mLoop (loop) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                       
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                        
                    Interpreter::Type_Float volume = runtime[0].mFloat;
                    runtime.pop();

                    Interpreter::Type_Float pitch = runtime[0].mFloat;
                    runtime.pop();
                        
                    context.getSoundManager().playSound3D (
                        context.getWorld().getPtr (id, true), sound, volume, pitch, mLoop);

                } 
        };     

        class OpStopSoundExplicit : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();                   
                    
                    std::string sound = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                            
                    context.getSoundManager().stopSound3D (
                        context.getWorld().getPtr (id, true), sound);
                } 
        };      
                                
        class OpGetSoundPlayingExplicit : public Interpreter::Opcode0
        {
            public:
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                                            
                    int index = runtime[0].mInteger;
                    runtime.pop();
                        
                    runtime.push (context.getSoundManager().getSoundPlaying (
                        context.getWorld().getPtr (id, true),
                        runtime.getStringLiteral (index)));
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

        const int opcodeSayExplicit = 0x2000019;
        const int opcodeSayDoneExplicit = 0x200001a;
        const int opcodePlaySound3DExplicit = 0x200001b;
        const int opcodePlaySound3DVPExplicit = 0x200001c;
        const int opcodePlayLoopSound3DExplicit = 0x200001d;
        const int opcodePlayLoopSound3DVPExplicit = 0x200001e;
        const int opcodeStopSoundExplicit = 0x200001f;
        const int opcodeGetSoundPlayingExplicit = 0x2000020;
        
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("say", "SS", opcodeSay, opcodeSayExplicit);
            extensions.registerFunction ("saydone", 'l', "", opcodeSayDone, opcodeSayDoneExplicit);
            extensions.registerInstruction ("streammusic", "S", opcodeStreamMusic);
            extensions.registerInstruction ("playsound", "c", opcodePlaySound);
            extensions.registerInstruction ("playsoundvp", "cff", opcodePlaySoundVP);
            extensions.registerInstruction ("playsound3d", "c", opcodePlaySound3D,
                opcodePlaySound3DExplicit);
            extensions.registerInstruction ("playsound3dvp", "cff", opcodePlaySound3DVP,
                opcodePlaySound3DVPExplicit);
            extensions.registerInstruction ("playloopsound3d", "c", opcodePlayLoopSound3D,
                opcodePlayLoopSound3DExplicit);
            extensions.registerInstruction ("playloopsound3dvp", "cff", opcodePlayLoopSound3DVP,
                opcodePlayLoopSound3DVPExplicit);
            extensions.registerInstruction ("stopsound", "c", opcodeStopSound,
                opcodeStopSoundExplicit);
            extensions.registerFunction ("getsoundplaying", 'l', "c", opcodeGetSoundPlaying,
                opcodeGetSoundPlayingExplicit);   
        }
        
        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5 (opcodeSay, new OpSay);
            interpreter.installSegment5 (opcodeSayDone, new OpSayDone);
            interpreter.installSegment5 (opcodeStreamMusic, new OpStreamMusic);
            interpreter.installSegment5 (opcodePlaySound, new OpPlaySound);
            interpreter.installSegment5 (opcodePlaySoundVP, new OpPlaySoundVP);
            interpreter.installSegment5 (opcodePlaySound3D, new OpPlaySound3D (false));
            interpreter.installSegment5 (opcodePlaySound3DVP, new OpPlaySoundVP3D (false));
            interpreter.installSegment5 (opcodePlayLoopSound3D, new OpPlaySound3D (true));
            interpreter.installSegment5 (opcodePlayLoopSound3DVP, new OpPlaySoundVP3D (true));
            interpreter.installSegment5 (opcodeStopSound, new OpStopSound);
            interpreter.installSegment5 (opcodeGetSoundPlaying, new OpGetSoundPlaying);
            
            interpreter.installSegment5 (opcodeSayExplicit, new OpSayExplicit);
            interpreter.installSegment5 (opcodeSayDoneExplicit, new OpSayDoneExplicit);            
            interpreter.installSegment5 (opcodePlaySound3DExplicit,
                new OpPlaySound3DExplicit (false));
            interpreter.installSegment5 (opcodePlaySound3DVPExplicit,
                new OpPlaySoundVP3DExplicit (false));
            interpreter.installSegment5 (opcodePlayLoopSound3DExplicit,
                new OpPlaySound3DExplicit (true));
            interpreter.installSegment5 (opcodePlayLoopSound3DVPExplicit,
                new OpPlaySoundVP3DExplicit (true));
            interpreter.installSegment5 (opcodeStopSoundExplicit, new OpStopSoundExplicit);
            interpreter.installSegment5 (opcodeGetSoundPlayingExplicit,
                new OpGetSoundPlayingExplicit);
        }
    }    
}

