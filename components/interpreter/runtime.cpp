
#include "runtime.hpp"

namespace Interpreter
{
    Runtime::Runtime (Context& context) : mContext (context), mCode (0) {}

    void Runtime::configure (const Interpreter::Type_Code *code, int codeSize)
    {    
        clear();
        
        mCode = code;
        mCodeSize = codeSize;
    }

    void Runtime::clear()
    {
        mCode = 0;
        mCodeSize = 0;
    }
}

