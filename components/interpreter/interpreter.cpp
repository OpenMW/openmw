
#include "interpreter.hpp"

namespace Interpreter
{
    Interpreter::Interpreter (Context& context)
    : mRuntime (context)
    {}
    
    Interpreter::~Interpreter()
    {
    
    }
    
    void Interpreter::run (const Type_Code *code, int codeSize)
    {
        mRuntime.configure (code, codeSize);
        
        
        mRuntime.clear();
    }
}
