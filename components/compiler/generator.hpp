#ifndef COMPILER_GENERATOR_H_INCLUDED
#define COMPILER_GENERATOR_H_INCLUDED

#include <vector>

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
    }
}

#endif

