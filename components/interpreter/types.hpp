#ifndef INTERPRETER_TYPES_H_INCLUDED
#define INTERPRETER_TYPES_H_INCLUDED

#include <cstdint>
#include <stdexcept>

namespace Interpreter
{
    typedef std::uint32_t Type_Code;

    typedef std::int16_t Type_Short;

    typedef std::int32_t Type_Integer;

    typedef float Type_Float;

    union Data
    {
        Type_Integer mInteger;
        Type_Float mFloat;
    };

    template <typename T>
    T& getData(Data& data)
    {
        throw std::runtime_error("unsupported data type");
    }

    template <>
    inline Type_Integer& getData(Data& data)
    {
        return data.mInteger;
    }

    template <>
    inline Type_Float& getData(Data& data)
    {
        return data.mFloat;
    }
}

#endif
