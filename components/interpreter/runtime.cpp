
#include "runtime.hpp"

#include <stdexcept>
#include <cassert>
#include <cstring>

namespace Interpreter
{
    Runtime::Runtime (Context& context) : mContext (context), mCode (0), mPC (0) {}
    
    int Runtime::getPC() const
    {
        return mPC;
    }
    
    int Runtime::getIntegerLiteral (int index) const
    {
        assert (index>=0 && index<static_cast<int> (mCode[1]));
    
        const Type_Code *literalBlock = mCode + 4 + mCode[0];
        
        return *reinterpret_cast<const int *> (&literalBlock[index]);
    }
            
    float Runtime::getFloatLiteral (int index) const
    {
        assert (index>=0 && index<static_cast<int> (mCode[2]));
    
        const Type_Code *literalBlock = mCode + 4 + mCode[0] + mCode[1];
        
        return *reinterpret_cast<const float *> (&literalBlock[index]);
    }
    
    std::string Runtime::getStringLiteral (int index) const
    {
        assert (index>=0 && index<static_cast<int> (mCode[3]));
    
        const char *literalBlock =
            reinterpret_cast<const char *> (mCode + 4 + mCode[0] + mCode[1] + mCode[2]);
    
        for (; index; --index)
        {
            literalBlock += std::strlen (literalBlock) + 1;
        }
    
        return literalBlock;
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
        mStack.clear();
    }
    
    void Runtime::setPC (int PC)
    {
        mPC = PC;
    }    
    
    void Runtime::push (Type_Data data)
    {
        mStack.push_back (data);
    }
    
    void Runtime::pop()
    {
        if (mStack.empty())
            throw std::runtime_error ("stack underflow");
            
        mStack.resize (mStack.size()-1);
    }
    
    Type_Data& Runtime::operator[] (int Index)
    {
        if (Index<0 || Index>=static_cast<int> (mStack.size()))
            throw std::runtime_error ("stack index out of range");
            
        return mStack[mStack.size()-Index-1];
    }
    
    Context& Runtime::getContext()
    {
        return mContext;
    }
}

