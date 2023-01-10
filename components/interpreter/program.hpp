#ifndef OPENMW_COMPONENTS_INTERPRETER_PROGRAM_H
#define OPENMW_COMPONENTS_INTERPRETER_PROGRAM_H

#include "types.hpp"

#include <string>
#include <vector>

namespace Interpreter
{
    struct Program
    {
        std::vector<Type_Code> mInstructions;
        std::vector<Type_Integer> mIntegers;
        std::vector<Type_Float> mFloats;
        std::vector<std::string> mStrings;
    };
}

#endif
