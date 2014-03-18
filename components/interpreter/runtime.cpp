
#include "runtime.hpp"

#include <stdexcept>
#include <cassert>
#include <cstring>

namespace Interpreter
{
    Runtime::Runtime() : mContext (0), mCode (0), mPC (0), mCodeSize(0) {}

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
        assert (index>=0 && static_cast<int> (mCode[3])>0);

        const char *literalBlock =
            reinterpret_cast<const char *> (mCode + 4 + mCode[0] + mCode[1] + mCode[2]);

        int offset = 0;

        for (; index; --index)
        {
            offset += std::strlen (literalBlock+offset) + 1;
            assert (offset/4<static_cast<int> (mCode[3]));
        }

        return literalBlock+offset;
    }

    void Runtime::configure (const Type_Code *code, int codeSize, Context& context)
    {
        clear();

        mContext = &context;
        mCode = code;
        mCodeSize = codeSize;
        mPC = 0;
    }

    void Runtime::clear()
    {
        mContext = 0;
        mCode = 0;
        mCodeSize = 0;
        mStack.clear();
    }

    void Runtime::setPC (int PC)
    {
        mPC = PC;
    }

    void Runtime::push (const Data& data)
    {
        mStack.push_back (data);
    }

    void Runtime::push (Type_Integer value)
    {
        Data data;
        data.mInteger = value;
        push (data);
    }

    void Runtime::push (Type_Float value)
    {
        Data data;
        data.mFloat = value;
        push (data);
    }

    void Runtime::pop()
    {
        if (mStack.empty())
            throw std::runtime_error ("stack underflow");

        mStack.resize (mStack.size()-1);
    }

    Data& Runtime::operator[] (int Index)
    {
        if (Index<0 || Index>=static_cast<int> (mStack.size()))
            throw std::runtime_error ("stack index out of range");

        return mStack[mStack.size()-Index-1];
    }

    Context& Runtime::getContext()
    {
        assert (mContext);
        return *mContext;
    }
}
