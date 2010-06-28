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
    
        void assignIntToLocal (CodeContainer& code, Literals& literals, char localType,
            int localIndex, int value);
    }
}

#endif

