#ifndef COMPILER_GENERATOR_H_INCLUDED
#define COMPILER_GENERATOR_H_INCLUDED

#include <vector>
#include <string>

#include <components/interpreter/types.hpp>

namespace Compiler
{
    class Literals;

    namespace Generator
    {
        typedef std::vector<Interpreter::Type_Code> CodeContainer;

        void pushInt (CodeContainer& code, Literals& literals, int value);

        void pushFloat (CodeContainer& code, Literals& literals, float value);

        void assignToLocal (CodeContainer& code, char localType,
            int localIndex, const CodeContainer& value, char valueType);
            
        void negate (CodeContainer& code, char valueType);
        
        void add (CodeContainer& code, char valueType1, char valueType2);

        void sub (CodeContainer& code, char valueType1, char valueType2);        
        
        void mul (CodeContainer& code, char valueType1, char valueType2);        
        
        void div (CodeContainer& code, char valueType1, char valueType2);        
        
        void convert (CodeContainer& code, char fromType, char toType);
        
        void squareRoot (CodeContainer& code);

        void exit (CodeContainer& code);        
        
        void message (CodeContainer& code, Literals& literals, const std::string& message,
            int buttons);
    }
}

#endif

