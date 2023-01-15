#include "literals.hpp"

#include <algorithm>
#include <cstring>

namespace Compiler
{
    int Literals::addInteger(Interpreter::Type_Integer value)
    {
        int index = static_cast<int>(mIntegers.size());

        mIntegers.push_back(value);

        return index;
    }

    int Literals::addFloat(Interpreter::Type_Float value)
    {
        int index = static_cast<int>(mFloats.size());

        mFloats.push_back(value);

        return index;
    }

    int Literals::addString(const std::string& value)
    {
        int index = static_cast<int>(mStrings.size());

        mStrings.push_back(value);

        return index;
    }

    void Literals::clear()
    {
        mIntegers.clear();
        mFloats.clear();
        mStrings.clear();
    }
}
