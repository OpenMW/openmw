#include "runtime.hpp"
#include "program.hpp"

#include <cassert>
#include <cstring>
#include <stdexcept>

namespace Interpreter
{
    int Runtime::getIntegerLiteral(int index) const
    {
        if (index < 0 || mProgram->mIntegers.size() <= static_cast<std::size_t>(index))
            throw std::out_of_range("Invalid integer index");

        return mProgram->mIntegers[static_cast<std::size_t>(index)];
    }

    float Runtime::getFloatLiteral(int index) const
    {
        if (index < 0 || mProgram->mFloats.size() <= static_cast<std::size_t>(index))
            throw std::out_of_range("Invalid float index");

        return mProgram->mFloats[static_cast<std::size_t>(index)];
    }

    std::string_view Runtime::getStringLiteral(int index) const
    {
        if (index < 0 || mProgram->mStrings.size() <= static_cast<std::size_t>(index))
            throw std::out_of_range("Invalid string literal index");

        return mProgram->mStrings[static_cast<std::size_t>(index)];
    }

    void Runtime::configure(const Program& program, Context& context)
    {
        clear();

        mContext = &context;
        mProgram = &program;
        mPC = 0;
    }

    void Runtime::clear()
    {
        mContext = nullptr;
        mProgram = nullptr;
        mStack.clear();
    }

    void Runtime::push(const Data& data)
    {
        mStack.push_back(data);
    }

    void Runtime::push(Type_Integer value)
    {
        Data data;
        data.mInteger = value;
        push(data);
    }

    void Runtime::push(Type_Float value)
    {
        Data data;
        data.mFloat = value;
        push(data);
    }

    void Runtime::pop()
    {
        if (mStack.empty())
            throw std::runtime_error("stack underflow");

        mStack.pop_back();
    }

    Data& Runtime::operator[](int index)
    {
        if (index < 0 || index >= static_cast<int>(mStack.size()))
            throw std::runtime_error("stack index out of range");

        return mStack[mStack.size() - index - 1];
    }

    Context& Runtime::getContext()
    {
        assert(mContext);
        return *mContext;
    }
}
