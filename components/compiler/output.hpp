#ifndef COMPILER_OUTPUT_H_INCLUDED
#define COMPILER_OUTPUT_H_INCLUDED

#include "literals.hpp"

#include <vector>

#include <components/interpreter/types.hpp>

namespace Compiler
{
    class Locals;

    class Output
    {
            Literals mLiterals;
            std::vector<Interpreter::Type_Code> mCode;            
            Locals& mLocals;
                
        public:
        
            Output (Locals& locals);
    
            void getCode (std::vector<Interpreter::Type_Code>& code) const;
            ///< store generated code in \a code.

            const Literals& getLiterals() const;

            const Locals& getLocals() const;
            
            const std::vector<Interpreter::Type_Code>& getCode() const;
            
            Literals& getLiterals();

            std::vector<Interpreter::Type_Code>& getCode();
            
            Locals& getLocals();
            
            void clear();
    };
}

#endif

