
#include "statsextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Stats
    {
        class OpGetAttribute : public Interpreter::Opcode0
        {
                int mIndex;
                
            public:
 
                OpGetAttribute (int index) : mIndex (index) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    Interpreter::Type_Integer value =
                        context.getReference().getCreatureStats().mAttributes[mIndex].
                        getModified();
                        
                    runtime.push (value);
                } 
        };  

        class OpGetAttributeExplicit : public Interpreter::Opcode0
        {
                int mIndex;
                
            public:
 
                OpGetAttributeExplicit (int index) : mIndex (index) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();
                    
                    Interpreter::Type_Integer value =
                        context.getWorld().getPtr (id, false).getCreatureStats().mAttributes[mIndex].
                        getModified();
                        
                    runtime.push (value);                        
                } 
        };  
        
        class OpSetAttribute : public Interpreter::Opcode0
        {
                int mIndex;
                
            public:
 
                OpSetAttribute (int index) : mIndex (index) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();
                   
                    context.getReference().getCreatureStats().mAttributes[mIndex].
                        setModified (value, 0);
                } 
        };  

        class OpSetAttributeExplicit : public Interpreter::Opcode0
        {
                int mIndex;
                
            public:
 
                OpSetAttributeExplicit (int index) : mIndex (index) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    context.getWorld().getPtr (id, false).getCreatureStats().mAttributes[mIndex].
                        setModified (value, 0);
                } 
        };
           
        class OpModAttribute : public Interpreter::Opcode0
        {
                int mIndex;
                
            public:
 
                OpModAttribute (int index) : mIndex (index) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());
                   
                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();
                    
                    value += context.getReference().getCreatureStats().mAttributes[mIndex].
                        getModified();
                   
                    context.getReference().getCreatureStats().mAttributes[mIndex].
                        setModified (value, 0, 100);
                } 
        };  

        class OpModAttributeExplicit : public Interpreter::Opcode0
        {
                int mIndex;
                
            public:
 
                OpModAttributeExplicit (int index) : mIndex (index) {}
            
                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    value +=
                        context.getWorld().getPtr (id, false).getCreatureStats().mAttributes[mIndex].
                        getModified();

                    context.getWorld().getPtr (id, false).getCreatureStats().mAttributes[mIndex].
                        setModified (value, 0, 100);                    
                } 
        };           
                
        const int numberOfAttributes = 8;

        const int opcodeGetAttribute = 0x2000027;
        const int opcodeGetAttributeExplicit = 0x200002f;
        const int opcodeSetAttribute = 0x2000037;
        const int opcodeSetAttributeExplicit = 0x200003f;
        const int opcodeModAttribute = 0x2000047;
        const int opcodeModAttributeExplicit = 0x200004f;
        
        void registerExtensions (Compiler::Extensions& extensions)
        {
            static const char *attributes[numberOfAttributes] =
            {
                "strength", "intelligence", "willpower", "agility", "speed", "endurance",
                "personality", "luck"
            };
        
            std::string get ("get");
            std::string set ("set");
            std::string mod ("mod");
        
            for (int i=0; i<numberOfAttributes; ++i)
            {
                extensions.registerFunction (get + attributes[i], 'l', "",
                    opcodeGetAttribute+i, opcodeGetAttributeExplicit+i);

                extensions.registerInstruction (set + attributes[i], "l",
                    opcodeSetAttribute+i, opcodeSetAttributeExplicit+i);

                extensions.registerInstruction (mod + attributes[i], "l",
                    opcodeModAttribute+i, opcodeModAttributeExplicit+i);                    
            }
        }        
        
        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            for (int i=0; i<numberOfAttributes; ++i)
            {
                interpreter.installSegment5 (opcodeGetAttribute+i, new OpGetAttribute (i));
                interpreter.installSegment5 (opcodeGetAttributeExplicit+i,
                    new OpGetAttributeExplicit (i));
                
                interpreter.installSegment5 (opcodeSetAttribute+i, new OpSetAttribute (i));
                interpreter.installSegment5 (opcodeSetAttributeExplicit+i,
                    new OpSetAttributeExplicit (i));
                
                interpreter.installSegment5 (opcodeModAttribute+i, new OpModAttribute (i));
                interpreter.installSegment5 (opcodeModAttributeExplicit+i,
                    new OpModAttributeExplicit (i));
            }
        }        
    }
}

