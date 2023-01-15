#include "output.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>

#include "locals.hpp"

namespace Compiler
{
    Output::Output(Locals& locals)
        : mLocals(locals)
    {
    }

    Interpreter::Program Output::getProgram() const
    {
        return Interpreter::Program{
            .mInstructions = mCode,
            .mIntegers = mLiterals.getIntegers(),
            .mFloats = mLiterals.getFloats(),
            .mStrings = mLiterals.getStrings(),
        };
    }

    const Literals& Output::getLiterals() const
    {
        return mLiterals;
    }

    const std::vector<Interpreter::Type_Code>& Output::getCode() const
    {
        return mCode;
    }

    const Locals& Output::getLocals() const
    {
        return mLocals;
    }

    Literals& Output::getLiterals()
    {
        return mLiterals;
    }

    std::vector<Interpreter::Type_Code>& Output::getCode()
    {
        return mCode;
    }

    Locals& Output::getLocals()
    {
        return mLocals;
    }

    void Output::clear()
    {
        mLiterals.clear();
        mCode.clear();
        mLocals.clear();
    }
}
