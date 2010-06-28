
#include "runtime.hpp"

namespace Interpreter
{
    Runtime::Runtime (Context& context) : mContext (context), mCode (0), mPC (0) {}
    
    int Runtime::getPC() const
    {
        return mPC;
    }
    
    void Runtime::configure (const Interpreter::Type_Code *code, int codeSize)
    {    
        clear();
        
        mCode = code;
        mCodeSize = codeSize;
        mPC = 0;
    }

    void Runtime::clear()
    {
        mCode = 0;
        mCodeSize = 0;
    }
    
    void Runtime::setPC (int PC)
    {
        mPC = PC;
    }    
}

